#include <iostream>
#include <assert.h>

#define ON 1
#define INSERTION_BALANCE ON	// 삽입 밸런싱 사용할지

#if INSERTION_BALANCE == ON
#define DELETION_BALANCE ON		// 삭제 밸런싱 사용할지
#endif

#define DeleteSafe(x)			\
do {							\
	if (x) {					\
		delete x;				\
	}							\
	x = nullptr;				\
} while (0);		


namespace AVL {

struct Node
{
	int Data = 0;
	Node* Left = nullptr;
	Node* Right = nullptr;
	Node* Parent = nullptr;

	Node* Any() {
		if (Left) {
			return Left;
		}

		if (Right) {
			return Right;
		}

		return nullptr;
	}

	int ChildCount() {
		if (Left && Right) {
			return 2;
		}

		if (Left || Right) {
			return 1;
		}

		return 0;
	}

	void DeleteChild(Node* node) {
		if (Left == node) {
			DeleteSafe(Left);
		} else if (Right == node) {
			DeleteSafe(Right);
		} else {
			assert("몽미?");
		}
	}
};

class AVLTree
{
public:
	AVLTree() {
		m_pVirtualRoot = new Node;
		m_iSize = 0;
	}
	~AVLTree() {
		Clear();
		DeleteSafe(m_pVirtualRoot);
	}
public:
	void Add(int data) {
		if (m_iSize == 0) {
			m_pVirtualRoot->Left = new Node{ data };
			m_pVirtualRoot->Left->Parent = m_pVirtualRoot;
			m_iSize++;
			return;
		}

		Node* pNewNode = Add(m_pVirtualRoot->Any(), data);
	#if INSERTION_BALANCE == ON
		BalanceStart(pNewNode);
	#endif
		m_iSize++;
	}

	bool Remove(int data) {
		Node* pDel = FindNode(data);	// 삭제할 노드를 찾는다.

		// 삭제할 노드를 못찾은 경우
		if (pDel == nullptr) {
			return false;
		}

		Node* pBalanceStartingNode = nullptr;	// 밸런싱을 진행할 노드 = 실질적으로 삭제된 노드의 부모 노드
		int iChildCount = pDel->ChildCount();

		if (iChildCount == 0) {
			pBalanceStartingNode = RemoveDelNoChild(pDel);
		} else if (iChildCount == 1) {
			pBalanceStartingNode = RemoveDelOneChild(pDel);
		} else {
			pBalanceStartingNode = RemoveDelTwoChild(pDel);
		}

	#if DELETION_BALANCE == ON
		if (pBalanceStartingNode)
			BalanceStart(pBalanceStartingNode);
	#endif
		m_iSize--;
		return true;
	}

	bool Find(int data) {
		return FindNode(data) != nullptr;
	}

	void Clear() {
		Node* pRoot = m_pVirtualRoot->Any();

		if (pRoot == nullptr) {
			return;
		}

		DeleteElementRecursive(pRoot->Left);
		DeleteElementRecursive(pRoot->Right);
		DeleteSafe(pRoot);
		m_iSize = 0;
	}
private:
	Node* Add(Node* root, int data) {
		Node* pParent = nullptr;
		Node* pCur = root;
		Node* pNewNode = new Node{ data };
		int iComp = 0;

		while (pCur != nullptr) {
			iComp = Compare(pCur->Data, data);
			pParent = pCur;
			pCur = iComp > 0 ? pCur->Right : pCur->Left;
		}

		if (iComp <= 0) {
			pParent->Left = pNewNode;
			pParent->Left->Parent = pParent;
		} else {
			pParent->Right = pNewNode;
			pParent->Right->Parent = pParent;
		}

		return pNewNode;
	}

	void DeleteElementRecursive(Node* node) {
		if (node == nullptr) {
			return;
		}

		DeleteElementRecursive(node->Left);
		DeleteElementRecursive(node->Right);
		DeleteSafe(node);
	}

	Node* RemoveDelNoChild(Node* del) {
		// 1. 삭제할 노드가 자식이 없는 경우
		//		부모에게서 해당 노드를 없애준다.
		Node* pParent = del->Parent;
		pParent->DeleteChild(del);
		return pParent;
	}

	Node* RemoveDelOneChild(Node* del) {
		// 2. 삭제할 노드가 자식이 한개인 경우
		//		1. 삭제할 노드가 부모기준 우측자식인 경우
		//			삭제할 노드의 자식을 삭제할 노드의 부모 우측에 붙여준다.
		//		[삭제 전]
		//		A - parent
		//			B - del
		//				C
		// 
		//		[삭제 후]
		//		A - parent
		//			C
		// 
		//		2. 삭제할 노드가 부모기준 좌측자식인 경우
		//			삭제할 노드의 자식을 삭제할 노드의 부모 좌측에 붙여준다.

		Node* pParent = del->Parent;
		Node* pChild = del->Any();
		Node* pBalanceStartingNode = nullptr;


		if (pParent == m_pVirtualRoot) {
			// 예외처리 만약 부모가 버추얼 노드라면 무조건 왼쪽에 붙이도록하자.
			// --> 처음 AVL트리 생성후 노드 삽입할 때 왼쪽에 무조건 첫 노드를 붙이도록 했기 때문
			pParent->Left = pChild;
		} else {
			if (pParent->Right == del) {
				pParent->Right = pChild;
			} else {
				pParent->Left = pChild;
			}
			pBalanceStartingNode = pParent;
		}

		pChild->Parent = pParent;
		DeleteSafe(del);
		return pBalanceStartingNode;
	}

	Node* RemoveDelTwoChild(Node* del) {
		// 3. 삭제할 노드가 자식이 2개인 경우
		//		1. 삭제할 노드의 좌측 자식의 서브트리에서 제일 큰 노드를 찾는다. (방법1)
		//			1. 제일 큰 노드의 값을 삭제할 노드의 값에 대입해준다.
		//			2. 제일 큰 노드의 좌측 자식을 제일 큰 노드의 부모의 우측 자식으로 만들어준다.
		//			3. 제일 큰 노드를 삭제해준다.
		// 
		//		2. 삭제할 노드의 우측 자식의 서브트리에서 제일 작은 노드를 찾는다. (방법2)
		//			1. 제일 작은 노드의 값을 삭제할 노드의 값에 대입해준다.
		//			2. 제일 작은 노드의 우측 자식을 제일 작은 노드의 부모의 좌측 자식으로 만들어준다.
		//			3. 제일 작은 노드를 삭제해준다.

		// --> 방법2로 진행

		Node* pSmallestParent = del;		// 삭제할 노드의 부모
		Node* pSmallest = del->Right;		// 삭제할 노드

		while (pSmallest->Left != nullptr) {
			pSmallest = pSmallest->Left;
		}

		pSmallestParent = pSmallest->Parent;
		del->Data = pSmallest->Data;

		if (pSmallestParent->Right == pSmallest) {
			// 우측에 제일 작은 노드가 달린경우 
			// 이 경우는 위의 while문을 한번도 안돌 경우이다.
			//   = "삭제할 노드보다 더 작은 노드가 아예 없는 경우"

			pSmallestParent->Right = pSmallest->Right;
			if (pSmallest->Right)
				pSmallest->Right->Parent = pSmallestParent;
		} else {
			pSmallestParent->Left = pSmallest->Right;
			if (pSmallest->Right)
				pSmallest->Right->Parent = pSmallestParent;
		}
		DeleteSafe(pSmallest);
		return pSmallestParent;
	}

	void BalanceStart(Node* node) {
		Node* pCur = node;

		while (pCur != m_pVirtualRoot) {
			Balance(pCur);
			pCur = pCur->Parent;
		}
	}

	// 해당 노드 기준으로 밸런싱 진행
	void Balance(Node* node) {
		int iHeightDiff = GetHeightDiff(node);
		// 우측 높이가 좌측 높이보다 2이상 높은 경우
		if (iHeightDiff >= 2) {
			int iChildHeightDiff = GetHeightDiff(node->Right);
			if (iChildHeightDiff >= 1) {
				RotateRR(node);
			} else {
				RotateRL(node);
			}
		} else if (iHeightDiff <= -2) {
			int iChildHeightDiff = GetHeightDiff(node->Left);
			if (iChildHeightDiff <= -1) {
				RotateLL(node);
			} else {
				RotateLR(node);
			}
		}
	}

	Node* FindNode(int data) {
		Node* pCur = m_pVirtualRoot->Any();

		while (pCur != nullptr) {
			if (pCur->Data == data) {
				return pCur;
			}

			if (pCur->Data < data) {
				pCur = pCur->Right;
			} else {
				pCur = pCur->Left;
			}
		}

		return nullptr;
	}

	int GetHeightDiff(Node* node) {
		int iLeftSubtreeHeight = GetHeight(node->Left, 0);
		int iRightSubtreeHeight = GetHeight(node->Right, 0);

		return iRightSubtreeHeight - iLeftSubtreeHeight;
	}

	int GetHeight(Node* cur, int height) {
		if (cur == nullptr) {
			return height;
		}
		int iLeftHeight = GetHeight(cur->Left, height + 1);
		int iRightHeight = GetHeight(cur->Right, height + 1);

		return iLeftHeight > iRightHeight ? iLeftHeight : iRightHeight;
	}


	void RotateRR(Node* cur) {
		//  ?   		- ? : pParent
		//    1 		- 1 : pCur
		//      3		- 3 : pChild
		//    ?	  5  	- ? : pChildLeft

		//  ?			- ? : pParent
		//    3			- 3 : pChild
		//  1   5		- 1 : pCur
		//   ?			- ? : pChildLeft

		Node* pParent = cur->Parent;
		Node* pCur = cur;
		Node* pChild = cur->Right;
		Node* pChildLeft = cur->Right->Left;

		if (pParent->Left == pCur)
			pParent->Left = pChild;
		else
			pParent->Right = pChild;
		pChild->Parent = pParent;


		pCur->Right = pChildLeft;
		if (pChildLeft)
			pChildLeft->Parent = pCur;

		pChild->Left = pCur;
		pCur->Parent = pChild;
	}

	void RotateLR(Node* cur) {
		RotateRR(cur->Left);
		RotateLL(cur);
	}

	void RotateRL(Node* cur) {
		RotateLL(cur->Right);
		RotateRR(cur);
	}

	void RotateLL(Node* cur) {
		//        ?		- pParent
		//      5		- pCur
		//    3			- pChild
		//  1   ?		- pChildRight

		//      ?		- pParent
		//    3			- pChild
		//  1   5		- pCur
		//    ?			- pChildRight

		Node* pParent = cur->Parent;
		Node* pCur = cur;
		Node* pChild = cur->Left;
		Node* pChildRight = cur->Left->Right;

		if (pParent->Left == pCur)
			pParent->Left = pChild;
		else
			pParent->Right = pChild;
		pChild->Parent = pParent;

		pCur->Left = pChildRight;
		if (pChildRight)
			pChildRight->Parent = pCur;

		pChild->Right = pCur;
		pCur->Parent = pChild;
	}


	int Compare(int src, int dst) {
		if (src < dst) {
			return 1;
		}

		if (src > dst) {
			return -1;
		}

		return 0;
	}


private:
	Node* m_pVirtualRoot;
	int m_iSize;
};

} // namespace AVL


int main() {
	AVL::AVLTree tree;

	tree.Add(1);
	tree.Add(2);
	tree.Add(3);
	tree.Add(4);
	tree.Add(5);

	std::cout << std::boolalpha;
	std::cout << "1 찾기 : " << tree.Find(1) << "\n";
	std::cout << "2 찾기 : " << tree.Find(2) << "\n";
	std::cout << "3 찾기 : " << tree.Find(3) << "\n";
	std::cout << "4 찾기 : " << tree.Find(4) << "\n";
	std::cout << "5 찾기 : " << tree.Find(5) << "\n";

	tree.Remove(1);
	tree.Remove(2);
	tree.Remove(3);
	tree.Remove(4);
	tree.Remove(5);


	return 0;
}


