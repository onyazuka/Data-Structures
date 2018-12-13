#pragma once
#include <memory>
#include <vector>
#include <iostream>

class vEBTreeCreationException {};

namespace vEBOperations
{
	int upSqrt(int x);
	int downSqrt(int x);

}

/*
	Implementation of van Emde Boas tree.
	With it we are able to to basic operations with maximum complexity of lglg(u)
		(where u is range (0...n), where n is maximum element of tree).
		So we must decide what u will be on creation stage.
	Operations:
		Minimum,
		Maxumum,
		Member,
		Successor,
		Predecessor,
		Insert,
		Delete.
	We are NOT doing template classes because it has not sense:
		van Emde Boas tree can be used only for integers.
		Technically, it can be used with chars, but here we can just use types cast.
*/

class vEBTree
{
	typedef std::unique_ptr<vEBTree> pvEBTree;
	typedef std::vector<pvEBTree> Clusters;
public:
	// when changing DataType, you must provide correct InvalidValue
	// ---bounded values
	typedef int DataType;
	enum { InvalidValue = -1 };
	// ---/bounded values

	vEBTree(int _u);
	vEBTree(int _u, DataType _min, DataType _max);

	// empty when both elements are invalid
	inline bool empty() const { return (min == max) && (max == InvalidValue); }
	inline bool hasOnlyOneElement() const { return (min == max) && !empty(); }
	inline DataType getMin() const { return min; }
	inline DataType getMax() const { return max; }

	//----main methods
	virtual bool contains(int x);
	virtual int predecessor(int x);
	virtual int successor(int x);
	virtual void insert(int x);
	virtual void erase(int x);
	//----/main methods

protected:

	inline void setMin(DataType newMin) { min = newMin; }
	inline void setMax(DataType newMax) { max = newMax; }
	inline int getU() const { return u; }

	//----index operations
	int high(int x);
	int low(int x);
	int index(int x, int y);
	//----/index operations

private:

	void createSubtrees(int u);
	void _insert(int x);
	void _erase(int x);
	void emptyTreeInsert(int x);

	pvEBTree summary;
	Clusters clusters;
	int u;
	DataType min;
	DataType max;
};
