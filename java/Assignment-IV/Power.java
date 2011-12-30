/**
 * 
 */

/**
 * @author Dvir
 *
 */
public class Power implements Expression {
	private Expression base;
	private double exponent;
	
	public Power(Expression base, double exponent) {
		this.base = base;
		this.exponent = exponent;
	}
	
	public Expression getBase() {
		return this.base;
	}
	
	public double getExponent() {
		return this.exponent;
	}
		
	/* (non-Javadoc)
	 * @see Expression#evaluate(Assignments)
	 */
	@Override
	public double evaluate(Assignments assignments) {
		return Math.pow(this.base.evaluate(assignments), this.exponent);
	}

	/* (non-Javadoc)
	 * @see Expression#derivative(Variable)
	 */
	@Override
	public Expression derivative(Variable var) {
		if (this.exponent == 0) {
			return new Constant(0);
		}
		
		double newExponent = this.exponent - 1;
		return new Multiplication(new Constant(this.exponent), new Power(this.base, newExponent));
	}
	
	public boolean equals(Expression other) {
		return (other != null && (other instanceof Power) && this.getBase() == other.getBase() && this.getExponent() == other.getExponent());
	}

	public String toString() {
		return "(" + this.base + "^" + this.exponent + ")";
	}
}