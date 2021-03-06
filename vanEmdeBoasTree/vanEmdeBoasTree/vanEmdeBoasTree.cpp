#include "stdafx.h"
#include "vanEmdeBoasTree.hpp"

//---------------------------vEB Operations---------------

int vEBOperations::upSqrt(int x)
{
	return (int)pow(2, ceil(log2(x) / 2));
}

int vEBOperations::downSqrt(int x)
{
	return (int)pow(2, floor(log2(x) / 2));
}

//--------------------------/vEB Operations---------------

vEBTree::vEBTree(int _u)
	: u{ _u }, min{ InvalidValue }, max{ InvalidValue }, summary { nullptr }
{
	// u must be power of 2 and < than 2!
	if (getU() < 2 || (getU() & (getU() - 1)) != 0)
	{
		throw vEBTreeCreationException{};
	}
	if (getU() == 2)	// do not need subtrees
	{
		return;
	}
	createSubtrees(getU());
}

vEBTree::vEBTree(int _u, DataType _min, DataType _max)
	: u{ _u }, min{ _min }, max{ _max }, summary{ nullptr }
{
	// u must be power of 2 and < than 2!
	if (getU() < 2 || (getU() & (getU() - 1)) != 0)
	{
		throw vEBTreeCreationException{};
	}
	if (getU() == 2)	// do not need subtrees
	{
		return;
	}
	createSubtrees(getU());
}

// index of subtree
int vEBTree::high(int x)
{
	return (int)floor((double)x / vEBOperations::downSqrt(u));
}

// index of cluster in subtree
int vEBTree::low(int x)
{
	return x % vEBOperations::downSqrt(u);
}

// restore index
int vEBTree::index(int x, int y)
{
	return x * vEBOperations::downSqrt(u) + y;
}

bool vEBTree::contains(int x)
{
	if (x == getMin() || x == getMax())
	{
		return true;
	}
	else if (getU() == 2)
	{
		return false;
	}
	else
	{
		return clusters[high(x)]->contains(low(x));
	}
}

int vEBTree::predecessor(int x)
{
	// case 1: we are in a base tree - can extract 
	// predecessor only if this = 1 and min = 0
	if (getU() == 2)
	{
		if (x == 1 && getMin() == 0)
		{
			return 0;
		}
		else
		{
			return InvalidValue;
		}
	}
	// case 2: x > maximum - returning maximum
	else if (getMax() != InvalidValue && x > getMax())
	{
		return getMax();
	}
	else
	{
		int minLow = clusters[high(x)]->getMin();
		// case 3: predecessor in this subtree
		if (minLow != InvalidValue && low(x) > minLow)
		{
			int offset = clusters[high(x)]->predecessor(low(x));
			return index(high(x), offset);
		}
		else
		{
			int predCluster = summary->predecessor(high(x));
			if (predCluster == InvalidValue)
			{
				// case 4: successor stored in minimum value here(not in cluster)
				if (getMin() != InvalidValue && x > getMin())
				{
					return getMin();
				}
				// case 5: not found successor
				return InvalidValue;
			}
			// case 6: found successor in successor subtree
			else
			{
				int offset = clusters[predCluster]->getMax();
				return index(predCluster, offset);
			}
		}
	}
}

int vEBTree::successor(int x)
{
	// case 1: we are in a base tree - can extract 
	// succesor only if this = 0 and max = 1
	if (getU() == 2)
	{
		if (x == 0 && getMax() == 1)
		{
			return 1;
		}
		else
		{
			return InvalidValue;
		}
	}
	// case 2: x < minimum - returning minumum
	else if (getMin() != InvalidValue && x < getMin())
	{
		return getMin();
	}
	else
	{
		int maxLow = clusters[high(x)]->getMax();
		// case 3: successor in this subtree
		if (maxLow != InvalidValue && low(x) < maxLow)
		{
			int offset = clusters[high(x)]->successor(low(x));
			return index(high(x), offset);
		}
		else
		{
			int succCluster = summary->successor(high(x));
			// case 4: not found successor
			if (succCluster == InvalidValue)
			{
				return InvalidValue;
			}
			// case 5: found successor in successor subtree
			else
			{
				int offset = clusters[succCluster]->getMin();
				return index(succCluster, offset);
			}
		}
	}
}

/*
	Checks "contains" status of x before inserting.
	(because we can not call insert if set already has this item).
	Complexity is not affected.
*/
void vEBTree::insert(int x)
{
	if (this->contains(x))
	{
		return;
	}
	_insert(x);
}

/*
	Checks "contains" status of x before erasing.
	(because we can not call erase if set not have this item).
	Complexity is not affected.
*/
void vEBTree::erase(int x)
{
	if (!(this->contains(x)))
	{
		return;
	}
	_erase(x);
}

/*
	Main method for inserting.
*/
void vEBTree::_insert(int x)
{
	int val = x;
	// case 1: empty tree - updating minimum and returning
	// (summary updating is not neede because minimum is NOT propagated to clusters).
	if (getMin() == InvalidValue)
	{
		emptyTreeInsert(val);
	}
	else
	{
		// case 2: new min - swapping x with min
		if (val < getMin())
		{
			int temp = getMin();
			setMin(val);
			val = temp;
		}
		// not base tree - inserting
		if (getU() > 2)
		{
			// case 3: inserting first element to cluster - need to update summary
			if (clusters[high(val)]->getMin() == InvalidValue)
			{
				summary->insert(high(val));
				clusters[high(val)]->emptyTreeInsert(low(val));
			}
			// case 4: inserting NEW element to cluster - not needed to update summary
			else
			{
				clusters[high(val)]->_insert(low(val));
			}
		}
		// if needed updating max
		if (val > getMax())
		{
			setMax(val);
		}
	}
}

/*
	Main method for erasing.
*/
void vEBTree::_erase(int x)
{
	int val = x;
	// case 1: only 1 element in vEB Tree
	if (getMin() == getMax())
	{
		setMin(InvalidValue);
		setMax(InvalidValue);
	}
	// case 2: deleting one of two elements from tree - no summary update needed
	else if (getU() == 2)
	{
		if (val == 0)
		{
			setMin(1);
		}
		else
		{
			setMin(0);
		}
		setMax(getMin());
	}
	else
	{
		// case 3: if deleting min - looking for new min and updating min value
		if (val == getMin())
		{
			int firstCluster = summary->getMin();
			val = index(firstCluster, clusters[firstCluster]->getMin());
			setMin(val);
		}
		// just deleting 
		clusters[high(val)]->_erase(low(val));
		// case 4: deleted last element from cluster - updating summary
		if (clusters[high(val)]->getMin() == InvalidValue)
		{
			summary->_erase(high(val));
			// updating max and summary max
			if (val == getMax())
			{
				int summaryMax = summary->getMax();
				// only min left - setting it as max
				if (summaryMax == InvalidValue)
				{
					setMax(getMin());
				}
				// else just finding new max from cluster
				else
				{
					setMax(index(summaryMax, clusters[summaryMax]->getMax()));
				}
			}
		}
		// cluster has not become empty - need to update max
		else if (val == getMax())
		{
			setMax(index(high(val), clusters[high(val)]->getMax()));
		}
	}
}

/*
	Used when constructing vEB Tree.
	Creates subtrees(for clusters or summaries) and populates them with invalid values
		(so it becomes empty).
	For tree with u we are creating upSqrt(u) clusters 
		and one summary(with size of upSqrt(u).
*/
void vEBTree::createSubtrees(int u)
{
	int upSqrtU = vEBOperations::upSqrt(u);
	int downSqrtU = vEBOperations::downSqrt(u);
	// case 1: new tree has u = 2 - creating vEBBaseTree
	
	for (int i = 0; i < upSqrtU; ++i)
	{
		clusters.push_back(pvEBTree(new vEBTree(
			downSqrtU, InvalidValue, InvalidValue)));
	}
	summary = pvEBTree (new vEBTree(
		upSqrtU, InvalidValue, InvalidValue));
}

void vEBTree::emptyTreeInsert(int x)
{
	setMin(x);
	setMax(x);
}


