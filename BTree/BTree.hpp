#pragma once
#include <vector>
#include <list>
#include <memory>

/*
	Node of B-Tree.
	Has such invariants:
		1. keys - increasing sequence;
		2. for every ki in keys vector:
			childer.ki <= this->keys[i];
		3. all leaves have the same height;
*/
template<typename Key>
class BTreeNode
{
public:
	typedef std::vector<Key> Keys;
	typedef std::shared_ptr<BTreeNode<Key>> pChild;
	typedef std::vector<pChild> Children;

	BTreeNode()
		: leaf{ false } {}

	inline void appendKey(Key k) { keys.push_back(k); }
	inline void prependKey(Key k) { keys.insert(keys.begin(), k); }
	inline void insertKey(int i, Key k) { keys.insert(keys.begin() + i, k); }
	inline auto findKey(Key k) { return std::find(keys.begin(), keys.end(), k); }
	// we have min keys = t - 1 and min children = t
	inline void resizeKeysAndChildren(int sz) { keys.resize(sz); children.resize(sz + 1); }
	inline Key& operator[](int i) { return keys[i]; }
	inline void eraseKey(int i) { keys.erase(keys.begin() + i); }
	inline pChild getChild(int i) const { return children[i]; }
	inline void setChild(int i, pChild ch) { children[i] = ch; }
	inline void appendChild(pChild ch) { children.push_back(ch); }
	inline void prependChild(pChild ch) { children.insert(children.begin(), ch); }
	inline void insertChild(int i, pChild ch) { children.insert(children.begin() + i, ch); }
	inline void eraseChild(int i) { children.erase(children.begin() + i); }
	inline int size() const { return keys.size(); }

	bool leaf;

private:
	Keys keys;
	Children children;
};

/*
	minDegree - minimal degree('t' in Cormen's book and later)
	We have such rules(ci = number of children for each node, ki = number of keys for each node):
		1. t <= ci <= 2t
		2. t-1 <= ki <= 2t-1
*/
template<typename Key>
class BTree
{
	typedef std::shared_ptr<BTreeNode<Key>> pNode;
	typedef std::pair<pNode, int> NodeIndexPair;
	typedef std::shared_ptr<NodeIndexPair> pNodeIndexPair;
public:
	BTree(int _minDegree);
	pNodeIndexPair search(Key key);
	void insert(Key key);
	void erase(Key key);
	pNode predecessor(pNode node, int keyIndex);
	pNode successor(pNode node, int keyIndex);
private:
	pNodeIndexPair _search(pNode searchNode, Key key);
	void _erase(pNode startNode, Key key);
	void insertNonfull(pNode node, Key key);
	void splitChild(pNode node, int i);
	pNode allocateNode();
	int normalizeNodeForErasing(pNode parentNode, int i);
	pNode unionNodesAroundKey(pNode left, Key key, pNode right);
	int diskRead(pNode node);
	int diskWrite(pNode node);
	int minDegree;
	pNode root;
};

template<typename Key>
BTree<Key>::BTree(int _minDegree)
	: minDegree{_minDegree}
{
	root = allocateNode();
	root->leaf = true;
	diskWrite(root);
}

/*
	Wraps private search with root as first arg.
*/
template<typename Key>
typename BTree<Key>::pNodeIndexPair BTree<Key>::search(Key key)
{
	return _search(root, key);
}

template<typename Key>
void BTree<Key>::insert(Key key)
{
	pNode curNode = root;
	// no space in root - need to do splitting and to make new root
	if (curNode->size() == (2 * minDegree - 1))
	{
		pNode newParent = allocateNode();
		root = newParent;
		newParent->leaf = false;
		newParent->appendChild(curNode);
		splitChild(newParent, 0);
		insertNonfull(newParent, key);
	}
	// enough space
	else
	{
		insertNonfull(curNode, key);
	}
}

template<typename Key>
void BTree<Key>::erase(Key key)
{
	if (root->size() == 0)	// empty tree
	{
		return;
	}
	_erase(root, key);
}


/*
	predecessor - rightmost ancestor of left sibling of 'node''s key with keyIndex
*/
template<typename Key>
typename BTree<Key>::pNode BTree<Key>::predecessor(pNode node, int keyIndex)
{
	pNode curNode = node->getChild(keyIndex);
	while (!curNode->leaf)
	{
		curNode = curNode->getChild(curNode->size());
		diskRead(curNode);
	}
	return curNode;
}

/*
	successor - leftmost ancestor of right sibling of 'node''s key with keyIndex
*/
template<typename Key>
typename BTree<Key>::pNode BTree<Key>::successor(pNode node, int keyIndex)
{
	pNode curNode = node->getChild(keyIndex + 1);
	while (!curNode->leaf)
	{
		curNode = curNode->getChild(0);
		diskRead(curNode);
	}
	return curNode;
}

/*
	Private version of search in B-Tree.
	For searching in full tree, call with root as first arg.
*/
template<typename Key>
typename BTree<Key>::pNodeIndexPair BTree<Key>::_search(pNode searchNode, Key key)
{
	int i = 0;
	// checking this node keys
	while (i < searchNode->size() && key > (*searchNode)[i])
	{
		++i;
	}
	// found one
	if (i < searchNode->size() && key == (*searchNode)[i])
	{
		NodeIndexPair nodeIndexPair = std::make_pair(searchNode, i);
		return std::make_shared<NodeIndexPair>(nodeIndexPair);
	}
	// not found and leaf
	else if (searchNode->leaf)
	{
		return nullptr;
	}
	// continue searching
	else
	{
		return _search(searchNode->getChild(i), key);
	}
}

/*
	Main function of erasing.
	startNode is hint where to start deleting
*/
template<typename Key>
void BTree<Key>::_erase(pNode startNode, Key key)
{
	// finding needed key or child where it should be
	pNode curNode = startNode;
	diskRead(curNode);
	int keyIndex = -1;
	do
	{
		bool found = false;
		pNode foundNode = nullptr;
		for (int i = 0; i < curNode->size(); ++i)
		{
			if ((*curNode)[i] == key)
			{
				keyIndex = i;
				found = true;
				foundNode = curNode;
				break;
			}
			else if ((*curNode)[i] > key)
			{
				// normalizing node for further traversing
				int deviation = normalizeNodeForErasing(curNode, i);
				i -= deviation;	// bad but as it is
				// traversing node is not changing after normalization(of course)
				curNode = curNode->getChild(i);
				diskRead(curNode);
				foundNode = curNode;
				break;
			}
		}
		// if not found - getting rightmost child of current node
		if (!foundNode) curNode = curNode->getChild(curNode->size());
		if (found)
		{
			break;
		}
	} while (!curNode->leaf);

	// case 1: node is leaf - simply erasing node
	if (curNode->leaf)
	{
		// searching for the last time - in leaf
		for (int i = 0; i < curNode->size(); ++i)
		{
			if ((*curNode)[i] == key)
			{
				keyIndex = i;
				break;
			}
		}
		if (keyIndex != -1)
		{ 
			curNode->eraseKey(keyIndex);
			curNode->resizeKeysAndChildren(curNode->size());
			diskWrite(curNode);
		}
		return;
	}
	// case 2: 
	pNode leftNode = curNode->getChild(keyIndex);
	pNode rightNode = curNode->getChild(keyIndex + 1);
	diskRead(leftNode);
	diskRead(rightNode);
	// 2.a - prior child has t or more keys
	if (leftNode->size() >= minDegree)
	{
		pNode predecessorNode = predecessor(curNode, keyIndex);
		// size - 1 because predecessor is always rightmost child
		Key swapKey = (*predecessorNode)[predecessorNode->size() - 1];
		_erase(predecessorNode, swapKey);
		diskWrite(predecessorNode);
		(*curNode)[keyIndex] = swapKey;
	}
	// 2.b - next child has t or more keys
	else if (rightNode->size() >= minDegree)
	{
		pNode successorNode = successor(curNode, keyIndex);
		// 0 because successor is always leftmost child
		Key swapKey = (*successorNode)[0];
		_erase(successorNode ,swapKey);
		diskWrite(successorNode);
		(*curNode)[keyIndex] = swapKey;
	}
	// 2.c - both prior and next children have t - 1 keys
	else
	{
		// 1. filling leftNode with key and rightNode's keys
		leftNode->appendKey(key);
		for (int i = 0; i < rightNode->size(); ++i)
		{
			leftNode->appendKey((*rightNode)[i]);
		}
		// 2. adding both of rightNode's children to the end of leftNode's children
		leftNode->appendChild(rightNode->getChild(0));
		leftNode->appendChild(rightNode->getChild(1));
		// 3. delete from curNode key and pointer to rightNode
		curNode->eraseKey(keyIndex);
		curNode->eraseChild(keyIndex + 1);
		// 4. free rightNode
		rightNode.reset();
		// 5. recursively erasing key from leftNode
		_erase(leftNode, key);
		diskWrite(leftNode);
		diskWrite(curNode);
		diskWrite(rightNode);
	}
}

/*
	Inserting if root is not full - main inserting function.
*/
template<typename Key>
void BTree<Key>::insertNonfull(pNode node, Key key)
{
	int i = node->size() - 1;
	// if leaf - simply searching for place to insert and inserting
	if (node->leaf)
	{
		node->resizeKeysAndChildren(node->size() + 1);
		// moving elements to right
		while (i >= 0 && key < (*node)[i])
		{
			(*node)[i + 1] = (*node)[i];
			--i;
		}
		(*node)[i + 1] = key;
		diskWrite(node);
	}
	// else - recursively going by tree, and sometimes, if needed,
	//		splitting it
	else
	{
		while (i >= 0 && key < (*node)[i]) { --i; }
		++i;
		// filling contents of i'th child of node
		diskRead(node->getChild(i));
		if (node->getChild(i)->size() == (2 * minDegree - 1))
		{
			splitChild(node, i);
			if (key > (*node)[i]) { ++i; }
		}
		insertNonfull(node->getChild(i), key);
	}
}

/*===================================================================
	   ------------					 -------------
	   |....N,W...|					 |... N S W..|
	   ------------					 -------------
			|			------>			  /  \
			|							 /    \
	------------------				--------- ---------
	|P Q R S T U V   |				| P Q R | | T U V |
	-----------------				--------- ---------
	
	splits full node as on illustration
	median goes to parent
	two children are splitted and now have t-1 elements each(before splitting
		node had 2t-1)
	if parent is also full, doing recursively

	Node x is splitted to two nodes - y and z
======================================================================
*/
template<typename Key>
void BTree<Key>::splitChild(pNode x, int i)
{
	int t = minDegree;
	pNode z = allocateNode();
	pNode y = x->getChild(i);
	z->leaf = y->leaf;
	z->resizeKeysAndChildren(t - 1);
	// moving first (t - 1) nodes of y to z
	for (int j = 0; j < (t - 1); ++j)
	{
		(*z)[j] = (*y)[j + t];
	}
	// setting z child
	if (!(y->leaf))
	{
		for (int j = 0; j < t; ++j)
		{
			z->setChild(j, y->getChild(j + t));
		}
	}
	Key keyGoingUp = (*y)[t - 1];
	y->resizeKeysAndChildren(t - 1);
	x->resizeKeysAndChildren(x->size() + 1);
	// moving x children after i + 1 to right
	for (int j = x->size() - 1; j >= i + 1; --j)
	{
		x->setChild(j + 1, x->getChild(j));
	}
	// inserting z - new child of x
	x->setChild(i + 1, z);
	for (int j = x->size() - 2; j >= i; --j)
	{
		(*x)[j + 1] = (*x)[j];
	}
	// inserting new key
	(*x)[i] = keyGoingUp;
	diskWrite(y);
	diskWrite(z);
	diskWrite(x);
}

/*
	Allocates node and page on disk for this node.
	NOW allocates only in memory.
*/
template<typename Key>
typename BTree<Key>::pNode BTree<Key>::allocateNode()
{
	pNode newNode = std::make_shared<BTreeNode<Key>>(BTreeNode<Key>());
	return newNode;
}

/*
	When erasing from B-Tree and traversing to next node,
	we should be sure that it has at least t(minDegree) keys.
	We can have 3 cases.
	Returns number for how many childIndex has changed(it can be 1 in 1 case, almost everytime it is 0).
*/
template<typename Key>
int BTree<Key>::normalizeNodeForErasing(pNode parentNode, int childIndex)
{
	int deviation = 0;
	// case 1: all right - node has t or more keys
	if (parentNode->getChild(childIndex)->size() >= minDegree)
	{
		return 0;
	}
	int keyIndex = (childIndex == 0) ? 0 : childIndex - 1;
	pNode normalizingNode = parentNode->getChild(childIndex);
	pNode leftNode =  parentNode->getChild(childIndex - 1);
	pNode rightNode = parentNode->getChild(childIndex + 1);
	diskRead(normalizingNode);
	diskRead(leftNode);
	diskRead(rightNode);
	/*
	case 2: left or right sibling has t or more keys:
		in this case we are inserting parent-key for normalizing node and sibling in norm node,
		then taking one(closest) key from this sibling and placing it as this parent's key,
		and updating parent's children vector.
	*/
	// 2.1. left sibling
	if (leftNode && leftNode->size() >= minDegree)
	{
		Key movingKey = (*parentNode)[keyIndex];
		parentNode->eraseKey(keyIndex);
		normalizingNode->prependKey(movingKey);
		movingKey = (*leftNode)[(leftNode->size() - 1)];
		parentNode->insertKey(keyIndex, movingKey);
		pNode movingChild = leftNode->getChild(leftNode->size());
		leftNode->eraseChild(leftNode->size());
		normalizingNode->prependChild(movingChild);
		leftNode->eraseKey(leftNode->size() - 1);
	}
	// 2.2 right sibling
	else if (rightNode && rightNode->size() >= minDegree)
	{
		Key movingKey = (*parentNode)[keyIndex];
		parentNode->eraseKey(keyIndex);
		normalizingNode->appendKey(movingKey);
		movingKey = (*rightNode)[0];
		parentNode->insertKey(keyIndex, movingKey);
		pNode movingChild = rightNode->getChild(0);
		rightNode->eraseChild(0);
		normalizingNode->appendChild(movingChild);
		rightNode->eraseKey(0);
	}
	/*
	case 3: left AND right siblings have t keys.
		In this case we are joining normalizing node with its sibling(left or right),
			(left here if presented, else right),
		and making their key-separator from parentNode this joined node's new median.
	*/
	else
	{
		pNode unionNode = nullptr;
		if (leftNode)
		{
			unionNode = unionNodesAroundKey(leftNode, (*parentNode)[keyIndex], normalizingNode);
			// because we have moved key to left
			deviation = 1;
		}
		else // if right node(else leaf and no such case)
		{
			unionNode = unionNodesAroundKey(normalizingNode, (*parentNode)[keyIndex], rightNode);
		}
		parentNode->eraseKey(keyIndex);
		parentNode->eraseChild(childIndex);
		// replacing instead of erasing
		parentNode->setChild(childIndex - 1, unionNode);
	}
	diskWrite(leftNode);
	diskWrite(rightNode);
	diskWrite(normalizingNode);
	diskWrite(parentNode);
	return deviation;
}

/*
	makes node left + right
	keys become: left.keys + key + right.keys
	children become: left.children + right.children
*/
template<typename Key>
typename BTree<Key>::pNode BTree<Key>::unionNodesAroundKey(pNode left, Key key, pNode right)
{
	pNode unionNode = allocateNode();
	unionNode->leaf = left->leaf && right->leaf;
	for (int j = 0; j < left->size(); ++j)
	{
		unionNode->appendKey((*left)[j]);
		unionNode->appendChild(left->getChild(j));
	}
	unionNode->appendChild(left->getChild(left->size()));
	unionNode->appendKey(key);
	for (int j = 0; j < right->size(); ++j)
	{
		unionNode->appendKey((*right)[j]);
		unionNode->appendChild(right->getChild(j));
	}
	unionNode->appendChild(right->getChild(right->size()));
	return unionNode;
}

/*
	Reading contents of next child node from disk.
	NOW it makes nothing.
	Returns error code, or 0 on success.
*/
template<typename Key>
int BTree<Key>::diskRead(pNode node)
{
	return 0;
}

template<typename Key>
int BTree<Key>::diskWrite(pNode node)
{
	return 0;
}