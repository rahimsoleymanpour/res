// exprnode/exprnode.h - Example expression tree
// 2004-02-25 Fred Swartz
#define NULL 0
#include <cstdlib>  // for NULL
#include <string>
#include <iostream>
#include <vector>
using namespace std;

//====================================== class ExprNode
class ExprNode {
public:
    //DFG nodes or Sorted nodes
	vector<int> _Output; //all outputs
	vector<int> _Parents; //all inputs
	vector<int> _ReOutput; //reconfigurable part
	vector<int> _ReParents; //reconfigurable part
    string		_Op;    // Operation ->>>> ADD(+), SUB(-), SHL(<), SHR(>), LOD(l), STR(s), MUL(*), DIV(/) <<<<-
	int         regFile; // reconfigurable
	int			_Id; //Id of each node
	int			_NumberOfOutputs; //number of outputs, each operation just have one output
	int         _NumberOfRealOutputs;
	int			_NumParents; //number of inputs to each node
	int			_UnconnectedInputs;
	int			_UnconnectedOutputs;
	double		_SwTime; //sw latency time
	double		_HwTime; //hw latency time
	double		_Area; //area of each node
	double		_DynamicPower; //power of nodes
	double		_immediateDynamicPower; //power of each node with immediate
	bool		_ForbiddenNode; //forbidden nodes ->>>> LD/ST
	int			_Order; //order of these node
	int         config_level;//for configuration
	int         config_row;//for configuration
	bool		traceFlag; //a flag for tracing
	int			lastID; //what is the last id of each node before topological sort
	int			level; //level of each node
	bool		flagLevel;//flag to check level of each node
	int			basicNum; //basic block number
	int			iteration; //what is the iteration of each node
	bool		hasCons; //whether it has constant
	bool		isTraced; //whether traced or not
};

class CI {
public:
	//store the Custom Instructions
	vector<int>		Nodes; //which nodes are include in CI
	vector<int>		roots; //which nodes are roots
	vector<int>		otherOp; //what is the other operation of each node
	vector<double>	otherMerit; //other merit
	vector<double>	oldotherMerit; //old other merit
	int				inputNum; //number of input
	int				outputNum; //number of output
	int				realOutputNum;
	int				Iteration; //number of iteration
	int				numNodes; //number of noeds in this CI
	bool			hasCons; //whether or not it has constant
	double			area; //area
	double			power; //power
	double			selfMerit; //what is self merit
	double			HWLatency;//Hardware Latency
	double			oldMerit;//old merit(another formula)
	double			merit; //total merit
	int				NumRep;//how many times this CI was seen
	vector<int>		otherIteration; //what is the other iteration of each math
};

class conflictNode {
public:
	//stores each conflict node
	int			Id; //identification
	vector<int> Nodes; //nodes
	vector<int> inSimilarities; //simillarity in each template
	vector<int> outSimilarities;//similarity out of each template
	int			TemNum; //template id
	int			inputNum; //number of input
	int			outputNum;//number of output
	int         realOutputNum;
	int			withTemp; //edges within template
	int			outTemp;//edges outside template
	double		Merit;//merit
	double		oldMerit;//another formula for merit
	double		Area;//area
	double		Power;//power
	int			Iteration;//iteration number
	double		HWLatency; //hw latency
	int			CommonNodes; //all edges
	bool		MISinclude;//whether or not in Maximum independent set
	bool		finalInclude;//is it in final Custom instruction set
	bool		hasCons; //whether it has constant
	double		weight;//weight this node based on merit
	double      tempWeight; //template weight
	double		firstWeight;//another template weight
	bool		checked;//flag to check
	bool		globalMIS;//is it in final MIS
};

/*class partialTree {
public:
	int					_nodeID;
	vector<partialTree*> _Parents;
	string				_Op;
	int					_NumParents;
};*/

class TLatency
{
public:
	double SWLatency;
	double HWLatency;
};

class TInOutNum{
public:
	int inNum;
	int outNum;
};
class reconfigNode
{
public:
	int num_parent;
	int parent[10];
	int num_childern;
	int childern[10];
	int typenode;// -1 = in , 0 = alu , +1 = cons


};
class typenode
{
public:
	
};