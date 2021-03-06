/**
 * 
 */
package ScienceCenter;

import java.util.*;
/**
 * @author Dvir, Or
 *
 */
public class Repository {
	private List<EquipmentPackage> _equipmentPackages;
	
	public Repository() {
		_equipmentPackages = new ArrayList<EquipmentPackage>();
	}
	
	/**
	 * Adds the equipment package to the repository in a lexicographic order
	 * @param ep The equipment package to add
	 */
	public synchronized void addEquipmentPackage(EquipmentPackage ep) {
		int i = 0;
		while (i < _equipmentPackages.size()) {
			EquipmentPackage curr = _equipmentPackages.get(i);
			if (curr.getName().compareTo(ep.getName()) > 0) {
				break;
			} else if (curr.getName().compareTo(ep.getName()) == 0) {
				// equal name packages! merge them and quit
				_equipmentPackages.get(i).increaseAmount(ep.getAmount());
			}
			
			i++;
		}
		
		_equipmentPackages.add(i, ep);
	}

	/**
	 * Takes equipment package from the repository, by a given name.
	 * 
	 * @param name The name of the equipment package.
	 * @return 	If we find the package (with the same @param name) in the repository, 
	 * 			we return the EquipmentPackage it represents.
	 *		    If we didn't find it, we return null.
	 */ 
	public EquipmentPackage searchRepository(String name) {
		for (int i = 0; i < _equipmentPackages.size(); ++i) {
			EquipmentPackage curr = _equipmentPackages.get(i);
			if (curr.getName().compareTo(name) == 0) {
				return curr;
			}
		}
		
		return null;
	}
	
	/**
	 * Returns the amount of a given package name.
	 * @param name The package name
	 * @return The amount found in the repository. Returns 0 if there is no such package.
	 */
	public int getAmount(String name) {
		ListIterator<EquipmentPackage> it = _equipmentPackages.listIterator();
		while (it.hasNext()) {
			EquipmentPackage curr = it.next();
			if (curr.getName() == name) {
				return curr.getAmount();
			}
		}
		
		return 0;	
	}
}
