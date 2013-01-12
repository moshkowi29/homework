#include "../include/ircsocket.h"

#include "../include/ui.h"
#include "../include/utils.h"
#include "../include/connectionHandler.h"
#include "../include/user.h"
#include "../include/channel.h"
#include "../include/message.h"

#include <string>

IRCSocket::IRCSocket(UI_ptr ui, User_ptr user) :
    _ch(),
    _ui(ui),
    _user(user)
{
}

IRCSocket::~IRCSocket() {
    delete _ch;
}
    
void IRCSocket::server(std::string host, unsigned short port) {
    _ch = new ConnectionHandler(host, port);
}

void IRCSocket::close() {
    this->_ch->close();
}

void IRCSocket::connect() {
    this->_ch->connect();
}

void IRCSocket::send(std::string message) {
#ifdef DBG
    this->_ui->history->addItem(
        Message_ptr(new Message(
            std::string("send -> ").append(message),
            Message::DEBUG
        ))
    );
#endif

    this->_ch->send(message);
}

void IRCSocket::read(std::string& message) {
    this->_ch->read(message);
}

IRCSocket::ServerMessage IRCSocket::parseServerMessage(std::string line) {
    IRCSocket::ServerMessage message;
    std::string prefix;

    // check if we got a prefix
    // if we do, set it and remove from the answer
    if (line.at(0) == ':') {
        prefix = line.substr(1, line.find(' '));
        // if the one who commited the action is a user,
        // his host will be: nick!user@host
        message.origin.raw = prefix;
        message.origin.nick = prefix.substr(0, prefix.find('!'));
        message.origin.user = prefix.substr(prefix.find('!')+1, prefix.find('@'));
        message.origin.host = prefix.substr(prefix.find('@')+1);
    
        line = line.substr(line.find(' ')+1);
    }

    // split to command parts
    Strings data = Utils::split(line, ':');
    Strings params = Utils::split(data[0], ' ');
    message.command = params[0]; 

    if (params.size() > 1) {
        message.target = params[1]; 
    }

    if (data.size() > 1) {
        // find the first occurrence of ':'
        // and fetch the whole string AFTER it
        message.text = line.substr(line.find(':')+1);
    }

    return message;
}

void IRCSocket::start() { 
    std::string answer;

    while (true) {
        try {
            // read from the socket. NOTE: blocking!
            this->read(answer);
        } catch (std::exception& e) {
            // connection termianted. stop cleanly
            this->_ui->history->addItem(
                Message_ptr(new Message(
                    "Disconnected from server.",
                    Message::SYSTEM
                ))
            );
            return;
        }

        answer = Utils::trim(answer);
      
        IRCSocket::ServerMessage message = this->parseServerMessage(answer);

#ifdef DBG
    this->_ui->history->addItem(
        Message_ptr(new Message(
            answer,
            Message::DEBUG
        ))
    );
#endif

        
        if (message.command == "JOIN") {
            if (message.origin.nick == this->_user->getNick()) {
                // joined a new channel!
                this->_ui->setChannel(Channel_ptr(new Channel(message.target)));

                this->_ui->history->addItem(
                        Message_ptr(new Message(
                            std::string("Now talking on ").append(message.target),
                            Message::SYSTEM
                            ))
                        );
            } else {
                // someone else joined a channel we are in
                // add him to the names list.
                User_ptr newUser(new User(message.origin.nick));
                this->_ui->addUser(newUser);
                this->_ui->history->addItem(
                        Message_ptr(new Message(
                            std::string("has joined ").append(message.target),
                            newUser,
                            Message::ACTION
                            ))
                        );
            }

        } else if (message.command == "PART") {
            if (message.origin.nick == this->_user->getNick()) {
                // quit channel. 
                this->_ui->setChannel(Channel_ptr());

                this->_ui->history->addItem(
                        Message_ptr(new Message(
                            std::string("You have left ").append(message.target),
                            Message::SYSTEM
                            ))
                        );
            } else {
                // someone quit the current channel.
                // find him on the names list and remove him.
                Users users = this->_ui->names->getList();
                for (size_t ii = 0; ii < users.size(); ++ii) {
                    if (message.origin.nick == users[ii]->getNick()) {
                        // found him! remove from the list
                        this->_ui->names->removeItem(ii);

                        std::string text("has left ");
                        text.append(message.target);
                        if (message.text != "") {
                            text.append(" (").append(message.text).append(")");
                        }
                        this->_ui->history->addItem(
                                Message_ptr(new Message(
                                    text,
                                    users[ii],
                                    Message::ACTION
                                    ))
                                );

                        break;
                    }
                }
            }

        } else if (message.command == "PRIVMSG") {
            if (NULL != this->_ui->getChannel() 
                    && message.target == this->_ui->getChannel()->getName()) {
                // message to current channel!
                this->_ui->history->addItem(
                        Message_ptr(new Message(
                            message.text,
                            User_ptr(new User(message.origin.nick))
                            ))
                        );
            } else if (message.target == this->_user->getNick()) {
                // a private message to the user
                this->_ui->history->addItem(
                        Message_ptr(new Message(
                            message.text,
                            User_ptr(new User(message.origin.nick)),
                            Message::PRIVATE 
                            ))
                        );
            }

        } else if (message.command == "QUIT") {
            // someone quit while being in the current channel.
            // find him on the names list and remove him.
            Users users = this->_ui->names->getList();
            for (size_t ii = 0; ii < users.size(); ++ii) {
                if (message.origin.nick == users[ii]->getNick()) {
                    // found him! remove from the list
                    this->_ui->names->removeItem(ii);

                    std::string text = "has quit";
                    if (message.text != "") {
                        text.append(" (").append(message.text).append(")");
                    }
                    this->_ui->history->addItem(
                            Message_ptr(new Message(
                                text,
                                users[ii],
                                Message::ACTION
                                ))
                            );

                    break;
                }
            }
        } else if (message.command == "NICK") {
            if (message.origin.nick == this->_user->getNick()) {
                // current user changed nick!
                // update current user
                this->_user->setNick(message.text);
            } else {
                // someone else changed nick.
                // find him in channel list and update his nick.
                Users users = this->_ui->names->getList();
                for (size_t ii = 0; ii < users.size(); ++ii) {
                    if (message.origin.nick == users[ii]->getNick()) {
                        // found the user. change his nick
                        // to the new nick.
                        users[ii]->setNick(message.text);
                        this->_ui->names->redraw();

                        this->_ui->history->addItem(
                                Message_ptr(new Message(
                                    std::string("is now known as ")
                                    .append(message.text),
                                    users[ii],
                                    Message::ACTION
                                    ))
                                );

                        break;
                    }
                }
            }

        } else if (message.command == "NOTICE") {
            if (NULL != this->_ui->getChannel() && 
                    message.target == this->_ui->getChannel()->getName()) {
                // message to current channel!
                this->_ui->history->addItem(
                        Message_ptr(new Message(
                            message.text,
                            User_ptr(new User(message.origin.nick))
                            ))
                        );
            } else if (message.target == this->_user->getNick()) {
                // a private notice to the user
                this->_ui->history->addItem(
                        Message_ptr(new Message(
                            message.text,
                            User_ptr(new User(message.origin.nick)),
                            Message::PRIVATE 
                            ))
                        );
            }

        } else if (message.command == "332" || message.command == "TOPIC") {
            // topic change
            // changing topic 
            this->_ui->getChannel()->setTopic(message.text); 
            this->_ui->title->redraw();

            this->_ui->history->addItem(
                    Message_ptr(new Message(
                        std::string("has changed the topic to: ")
                        .append(message.text),
                        User_ptr(new User(message.origin.nick)),
                        Message::ACTION
                        ))
                    );
        } else if (message.command == "353") { // names list
            // this will start a names stream 
            // only if there is none active
            this->_ui->startNamesStream();
            this->_ui->addNames(message.text);

        } else if (message.command == "366") { // end names list
            // stop streaming names to the ui.
            // this will set the names list.
            this->_ui->endNamesStream();

        } else if (message.command == "001"
                || message.command == "002"
                || message.command == "003"
                || message.command == "004"
                || message.command == "005"
                || message.command == "251"
                || message.command == "252"
                || message.command == "254"
                || message.command == "255"
                || message.command == "372"
                || message.command == "372"
                || message.command == "375"
                || message.command == "376"
                ) {
            this->_ui->history->addItem(
                    Message_ptr(new Message(
                        message.text,
                        Message::SYSTEM
                        ))
                    );
        } else if (message.command == "PING") {
            std::ostringstream response;
            response << "PONG :" << message.text;
            _ch->send(response.str());
        } else if (message.command == "NOTICE AUTH") {
            this->_ui->history->addItem(
                Message_ptr(
                    new Message(
                        message.text,
                        Message::SYSTEM
                    )
                )
            );
        }

        answer.clear();
    }
}

IRCSocket::ClientCommand IRCSocket::parseClientCommand(std::string line) {
    IRCSocket::ClientCommand command;
   
    if (line.find(' ') != std::string::npos) {
        command.params = line.substr(line.find(' ')+1);
    }

    if (line.at(0) == '/') {
        command.name = line.substr(1, line.find(' ')-1);
    } else {
        command.name = "chanmsg";
    }
    
    return command;
}

std::string IRCSocket::clientCommand(std::string line) {
    // this is a command! parse it
    IRCSocket::ClientCommand command = IRCSocket::parseClientCommand(line);

    if (command.name == "nick") {
        // changing nick.
        return nick(command.params);

    } else if (command.name == "join") {
        // join a new channel
        return join(command.params);

    } else if (command.name == "part") {
        return part(command.params);
        
    } else if (command.name == "topic") {
        return topic(command.params);

    } else if (command.name == "quit") {
        return quit(command.params);

    } else if (command.name == "msg") {
        return message(command.params);

    } else {
        // unknown command.
        throw (
            std::logic_error(
                std::string("Unknown command '")
                .append(command.name)
                .append("'")
            )
        );
    }

    return "";
}

void IRCSocket::auth(std::string nick, std::string name) {
    this->send(this->nick(nick));
    this->send(this->user(nick));
}

std::string IRCSocket::nick(std::string params) {
    return std::string("NICK ").append(params);
}

std::string IRCSocket::user(std::string nick, std::string name) {
    return std::string("USER ")
            .append(nick)
            .append(" 0 * :")
            .append(name);
}

std::string IRCSocket::user(std::string params) {
    return std::string("USER ")
            .append(params)
            .append(" 0 * :")
            .append(params);
}

std::string IRCSocket::join(std::string params) {
    return std::string("JOIN ").append(params);
}

std::string IRCSocket::part(std::string params) {
    return std::string("PART ").append(params);
}

std::string IRCSocket::topic(std::string params) {
    std::string channel = params.substr(0, params.find(' '));
    
    std::string response("TOPIC ");
    response.append(channel);

    std::string topic;
    if (params.find(' ') != std::string::npos) {
        topic = params.substr(params.find(' ')+1);

        // topic is given - append it to response
        // NOTE: empty topic is fine; that means it would be cleared.
        response.append(" :").append(topic);
    }

    return response; 
}

std::string IRCSocket::quit(std::string params) {
    std::string response("QUIT");
    if (params != "") {
        response.append(" :").append(params);
    }

    return response;
}

std::string IRCSocket::message(std::string target, std::string text) {
    std::string response("PRIVMSG ");
    response.append(target).append(" :").append(text);
    
    return response;
}

std::string IRCSocket::message(std::string params) {
    std::string target = params.substr(0, params.find(' '));
    std::string msg;
    if (params.find(' ') != std::string::npos) {
        // message is given - append it to response
        // NOTE: we let the server deal with empty messages.
        
        msg = params.substr(params.find(' ')+1);
    }

    return this->message(target, msg);
}