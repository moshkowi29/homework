/**
 * 
 */

/**
 * @author Dvir
 *
 */
public class Subtraction implements Expression {
	private Expression x, y;
	
	public Subtraction(Expression x, Expression y) {
		if (x == null || y == null) {
			throw new RuntimeException("Subtraction.Subtraction() received a null expression.");
		}			
		
		this.x = x;
		this.y = y;
	}
	
	public Expression getX() {
		return this.x;
	}
	
	public Expression getY() {
		return this.y;
	}	
	
	/* (non-Javadoc)
	 * @see Expression#evaluate(Assignments)
	 */
	@Override
	public double evaluate(Assignments assignments) {
		return this.x.evaluate(assignments) - this.y.evaluate(assignments);
	}

	/* (non-Javadoc)
	 * @see Expression#derivative(Variable)
	 */
	@Override
	public Expression derivative(Variable var) {
		if (var == null) {
			throw new RuntimeException("Subtraction.derivative() received a null variable.");
		}				
		
		return new Subtraction(this.x.derivative(var), this.y.derivative(var));
	}
	
	public boolean equals(Object other) {
		return (other != null && (other instanceof Subtraction) && this.getX().equals(((Subtraction)other).getX()) && this.getY().equals(((Subtraction)other).getY()));		
	}

	public String toString() {
		return "(" + this.x + "-" + this.y + ")";
	}
}
