#define DBG

#include <curses.h>
#include <iostream>
#include <sstream>
#include <istream>
#include <ostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include "../include/typedef.h"

#include "../include/connectionHandler.h"
#include "../include/user.h"
#include "../include/channel.h"
#include "../include/message.h"
#include "../include/ircsocket.h"
#include "../include/ui.h"
#include "../include/window.h"
#include "../include/contentwindow.h"
#include "../include/listwindow.h"
#include "../include/inputwindow.h"
#include "../include/utils.h"

void debug (ListWindow<Message_ptr>* history, std::string message) {
    history->addItem(
        Message_ptr(
            new Message(
                message, 
                User_ptr(new User("debug")) 
            )
        )
    );
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " host port nick" << std::endl;
        return 1;
    }

    std::string host(argv[1]);
    unsigned short port = atoi(argv[2]);
    std::string nick(argv[3]);

    boost::thread_group thread_group;
    
    User_ptr user(new User(nick));

    int ch = 0;

    initscr();          /* Start curses mode        */
    start_color();          /* Start the color functionality */
    cbreak();           /* Line buffering disabled, Pass on
                         * everty thing to me       */
    keypad(stdscr, TRUE);       /* I need that nifty F1     */
    noecho();

    assume_default_colors(COLOR_GREEN, COLOR_BLACK);
    
    InputWindow* wInput = new InputWindow("input", 3, 153, 45, 0);
    ListWindow<User_ptr>* wNames = new ListWindow<User_ptr>("names", 46, 17, 2, 152);
    ContentWindow<Channel_ptr>* wTitle = new ContentWindow<Channel_ptr>("title", 3, 169, 0, 0);
    ListWindow<Message_ptr>* wHistory = new ListWindow<Message_ptr>("main", 44, 153, 2, 0);
   
    // UI is a set of title, history, names and input windows.
    UI_ptr ui(new UI(wTitle, wHistory, wNames, wInput));
    
    ui->names->addRefreshAfterWindow(wInput);
    ui->names->setVisibleSize(44);
    ui->names->setReverseList(false);
    ui->names->addItem(user);
    
    ui->title->addRefreshAfterWindow(wInput);

    ui->history->addRefreshAfterWindow(wInput);
    ui->history->setVisibleSize(42);
 
    IRCSocket server(ui, user);
    
    while (true) {
        server.server(host, port);
        
        ui->history->addItem(
            Message_ptr(new Message(
                "Connecting to server...",
                Message::SYSTEM
            ))
        );
        try {
            server.connect();
        } catch (std::exception& e) {
            ui->history->addItem(
                Message_ptr(new Message(
                    "Connection failed!",
                    Message::SYSTEM
                    ))
            );
            
            break;
        }

        ui->history->addItem(
            Message_ptr(new Message(
                "Authenticating...",
                Message::SYSTEM
            ))
        );
        try {
            server.auth(user->getNick(), user->getName());
        } catch (std::exception& e) {
            ui->history->addItem(
                Message_ptr(new Message(
                    "Connection lost.",
                    Message::SYSTEM
                    ))
            );
            
            break;
        }
       
        // start server socket thread to handle data from the server.
        // allows for non-blocking stdin.
        boost::thread* serverSocketThread = new boost::thread(
                &IRCSocket::start, 
                &server 
        );

        thread_group.add_thread(serverSocketThread);

        break;
    }
    
    bool exit = false;
    std::string line;
    do {
        switch(ch) {   
            case 0: // first run!
                // do nothing here for now.
                break;

            case 263: // backspace
                ui->input->deleteLastChar();
                break;

            case 10: { // send message to server!
                
                line = ui->input->str();
                ui->input->clearInput(); 
                
                if (line.size() == 0) {
                    // empty message; don't do anything
                    break;
                }
                
                std::string response;

                try {
                        
                    IRCSocket::ClientCommand command = IRCSocket::parseClientCommand(line);

                    // if it's any of client only commands, handle here
                    // and don't send to the protocol.
                    if (command.name == "clear") {
                        ui->history->removeAll();
                    } else if (command.name == "raw") { // allow sending raw messages to server
                        response = command.params;
                    } else if (command.name == "debug") { // toggle debug messages
                        //_debug = !_debug;
                    } else if (command.name == "exit") {
                        // exit client cleanly!
                        ui->history->addItem(
                            Message_ptr(new Message(
                                "Shutting down...",
                                Message::SYSTEM
                            ))
                        );
                        
                        // first, disconnect from server if we are logged in.
                        response = server.clientCommand("/quit");

                        // stop stdin loop
                        exit = true;
                    } else if (command.name == "server") {
                        // change servers.

                    } else if (command.name == "chanmsg") { 
                        // if we got here, this is a message.
                        // check if we are already in a channel
                        if (NULL == ui->getChannel()) {
                            // no active channel! 
                            // alert the user that he should join first
                            throw (
                                std::logic_error(
                                    "Can't send message - no active channels."
                                )
                            );
                        }
                
                        // put it in the channel history list
                        Message_ptr message(new Message(line, user));
                        ui->history->addItem(message);

                        // transmit message to the server
                        response = server.message(
                            std::string(ui->getChannel()->getName())
                            .append(" ")
                            .append(line)
                        );
                    } else {
                        // not any of client only commands.
                        // call the protocol.

                        response = server.clientCommand(line);
                    }

                } catch (std::exception& error) {
                    // display error to client
                    ui->history->addItem(
                        Message_ptr(
                            new Message(
                                error.what(),
                                Message::ERROR
                            )
                        ) 
                    );

                }

                if (response != "") {
                    server.send(response);
                }

                break; // end enter case
            }

            default:
                if (ch >= 32 && ch <= 126) { // valid ascii character
                    ui->input->putChar(ch);
                }
                break;
        } 
    } while (!exit && (ch = ui->input->getChar()) != KEY_END);

    // wait for all threads to finish
    thread_group.join_all();

    // end curses mode
    endwin();

    // clean
    delete wInput;
    delete wHistory;
    delete wNames;
    delete wTitle;

    return 0;
}