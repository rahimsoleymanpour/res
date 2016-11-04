#include "test.h"
#include "basicFunction.h"
#include "precision_timer.h"
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>
#include <string>

//----------------------------------
int changeable = 0;
//vector<CI*> CIarray;
int verilogGenerated = 0;
CI* CIarray[900000];
string genCombineResults;
string genFuncName;
string genAlgName;
ofstream CIall;
ofstream RFU_profile4size;
#define MaxNumoflevel_RFU 4
//#define MaxNumofrow_RFU 10
#define MaxNumofinput_RFU 4
#define MaxNumofoutput_RFU 3
#define ALU_RFU 0.7662
#define MUX_RFU 0.0312
#define clock_number 3
double mul_HWdelay = 0.0312;
bool RISP = false;
int MaxnumofRow_RFU[10]={4,3,2,1,1,1,1,1,1};

//-------------------------------------------------------------------------------------------------
bool findSimilarities(vector<int> & baseNodes, vector<int> & tmpNodes)
{//begin of findSimilarities(checks if there are any nodes that are in both of CI)
	//1)baseNodes				a boolean array that shows the custom instruction
	//2)tmpNodes		an array that contains node's information that are in topological order
	//---------------------------------------------------------------------------------------------
	
	//definition of variables------------
	bool NumberSim = false;
	unsigned int i, j;
	//-----------------------------------

	for(i = 0; i < baseNodes.size(); i++)
	{
		for(j = 0; j < tmpNodes.size(); j++)
		{
			if(baseNodes[i] == tmpNodes[j])
			{
				break;
			}
		}
		if(j != tmpNodes.size())
		{
			NumberSim = true;
			break;
		}
	}
	if(NumberSim)
		return true;
	else
		return false;
}
//----------------------------------------------SOURCE NODE ALLOCATION----------------------------------
ExprNode* SourceNodeAllocation(bool *CIcand, ExprNode* SortedNodes, int DFGSize, int& SourceNum)
{//begin of SourceNodeAllocation(finds the source nodes of the structure)
	//1)CIcand:				a boolean array that shows the custom instruction
	//2)SortedNodes:		an array that contains node's information that are in topological order
	//3)DFGSize:			size of input DFG
	//---------------------------------------------------------------------------------------------
	
	//definition of variables---------
	ExprNode *SourceNodes;
	int VectorTemp[1000];
	int VectorTempIndex = 0;
	bool Add = true;
	SourceNodes = new ExprNode[100];
	//--------------------------------

	//checks all nodes for basic inputs of custom instructions
	for(int i=0; i< DFGSize; i++)
	{
		if(CIcand[i] == 1)
		{
			Add = true;
			for(int j=0; j< SortedNodes[i]._NumParents; j++)
			{
				if(CIcand[SortedNodes[i]._Parents[j]] == 1)
				{
					Add =false;
					break;
				}
			}
			if(Add)
			{
				VectorTemp[VectorTempIndex] = i;
				VectorTempIndex++;
			}

		}
	}
	
	//return the number of basic inputs of CI
	SourceNum = VectorTempIndex;
	//-----------------------------------------
	
	//adds basic inputs to the array of SourceNodes-----
	for(int i=0; i< VectorTempIndex; i++)
			SourceNodes[i] = SortedNodes[VectorTemp[i]];
	//---------------------------------------------------
	return SourceNodes;
}
//-----------------------------------------------------------------------------------
//find latency of custom instruction tree with dfs
double DFS(int NodeIndex,bool* CIcand, double latency, bool HW, ExprNode* SortedNodes)
{//begin of DFS(trace in depth-first-search manner)
	//1)NodeIndex:			index of node that should trace
	//2)CIcand:				a boolean array that shows the custom instruction
	//3)latency:			it shows the summation of the latency of traced nodes
	//4)HW:					it shows that we should calculate HW or SW latency
	//5)SortedNodes:		an array that contains node's information that are in topological order
	//---------------------------------------------------------------------------------------------
	
	if (NodeIndex < 0 )
		NodeIndex=0;

	latency = (HW) ? latency + SortedNodes[NodeIndex]._HwTime : latency + SortedNodes[NodeIndex]._SwTime;

	//definition of variables
	bool Add = true;
	double retLatency = 0;
	double temp;
	//-----------------------

	//if this node is the primary output of CI it returns the summation of latency
	for(int j=0; j< SortedNodes[NodeIndex]._NumberOfOutputs; j++)
	{
		if(CIcand[SortedNodes[NodeIndex]._Output[j]] == 1)
		{
			Add =false;
			break;
		}
	}
	if(Add)
		return latency;
	//---------------------------------------------------------------------------

	//calls depth-first-search on all outputs of this node and return the biggest one---
	for(int i=0 ; i< SortedNodes[NodeIndex]._NumberOfOutputs; i++)
	{
		if (CIcand[SortedNodes[NodeIndex]._Output[i]] == 0)
			continue;
		temp = DFS(SortedNodes[NodeIndex]._Output[i], CIcand, latency, HW, SortedNodes);
		if ( temp > retLatency)
			retLatency = temp;
	}
	//----------------------------------------------------------------------------------
	return retLatency;
}
//-------------------------------------------------------------------------------------
TLatency FindGraphLatency(bool* CIcand, ExprNode* SortedNodes, int DFGSize)
{//begin of FindGraphLatency(calculate critical path of CI)
	//1)CIcand:				a boolean array that shows the custom instruction
	//1)SortedNodes:		an array that contains node's information that are in topological order
	//2)DFGSize:			size of input DFG
	//---------------------------------------------------------------------------------------------
	 
	//definition of variables--
	double MaxLatency = 0;
	double temp = 0;
	TLatency MaxSWHWLatency;
	int SourceNum;
	//-------------------------
	
	//calls another function that ...
	ExprNode* SourceNode = SourceNodeAllocation(CIcand, SortedNodes, DFGSize, SourceNum);
	//------------------------------------------------------------------------------------
	for(int j=0; j< SourceNum; j++)
	{
		if(SourceNode[j]._NumberOfOutputs == 0)
		{
			temp = DFS(SourceNode[j]._Order, CIcand, 0, true, SortedNodes);
			if (temp > MaxLatency)
				MaxLatency = temp;
		}
		for(int i=0; i< SourceNode[j]._NumberOfOutputs; i++)
		{
			temp = DFS(SourceNode[j]._Order, CIcand, 0, true, SortedNodes);
			if (temp > MaxLatency)
				MaxLatency = temp;
									
		}
		
	}
	MaxSWHWLatency.HWLatency = MaxLatency;
	
	MaxSWHWLatency.SWLatency = 0;
	for(int i=0; i< DFGSize; i++)
		if (CIcand[i] == 1)
		{
			MaxSWHWLatency.SWLatency += SortedNodes[i]._SwTime;
			//printf("SW %d  HW  %d\n", MaxSWHWLatency.SWLatency , MaxSWHWLatency.HWLatency);
		}

	

	delete []SourceNode;
	SourceNode = NULL;
	return MaxSWHWLatency;
}
//------------------------------------------MERIT-----------------------------------------------
double FindMeritValue(bool *CIcand, ExprNode *SortedNodes, int DFGSize, double& HwLate)
{//begin of FindMeritValue(calculate merit of CIs(SWLatency - HWLatency))
	//1)CIcand:				a boolean array that shows the custom instruction
	//1)SortedNodes:		an array that contains node's information that are in topological order
	//2)DFGSize:			size of input DFG
	//---------------------------------------------------------------------------------------------

	//call another function that calculate the latency of custom instruction
	TLatency latency = FindGraphLatency(CIcand, SortedNodes, DFGSize);
	//----------------------------------------------------------------------
	double Merit = (latency.SWLatency - ceil(latency.HWLatency));
	
	//it also calculate the hardware latency of the CI
	HwLate = latency.HWLatency;
	//------------------------------------------------
	return ((latency.SWLatency == 0)||(latency.HWLatency == 0))? 0 : Merit;
}
//------------------------------------------OLD MERIT-------------------------------------------
//calculate merit of CIs(SWLatency / HWLatency)
double oldMeritValue(bool *CIcand, ExprNode *SortedNodes, int DFGSize)
{//begin of oldMeritValue
	//1)CIcand:				a boolean array that shows the custom instruction
	//2)SortedNodes:		an array that contains node's information that are in topological order
	//3)DFGSize:			size of input DFG
	//----------------------------------------------------------------------------------------------------------------
	
	//call another function that calculate the latency of custom instruction
	TLatency latency = FindGraphLatency(CIcand, SortedNodes, DFGSize);
	//----------------------------------------------------------------------

	//it calculates merit based on an old formula(e.g. SWLatency / HWLatency)
	double Merit = (latency.SWLatency / ceil(latency.HWLatency));
	return ((latency.SWLatency == 0)||(latency.HWLatency == 0))? 0 : Merit;
	//----------------------------------------------------------------------
}
//-----------------------------------------------------------------------------------------------
//it checks if CI has nodes that belongs to differnt basic blocks
bool findConflict(bool* CIcand,ExprNode* SortedNodes, int DFGSize)
{//begin of findConflict
	//1)CIcand:				a boolean array that shows the custom instruction
	//1)SortedNodes:		an array that contains node's information that are in topological order
	//2)DFGSize:			size of input DFG
	//----------------------------------------------------------------------------------------------------------------
	
	//definiton of variables-------
	int BB;
	bool flag = false;
	//-----------------------------

	//checks if no node come from different basic block 
	for(int i=0; i < DFGSize ; i++)
	{
		if(CIcand[i] && flag == false)
		{
			BB = SortedNodes[i].basicNum;
			flag = true;
		}
		if(CIcand[i] && flag == true)
		{
			if(BB != SortedNodes[i].basicNum)
				return true;
		}
	}
	//------------------------------------------------

	return false;
}

//----------------------------------------------------------------------------------------
void fillOrder(ExprNode* inputNodes, int DFGSize)
{//begin of fillOrder
	//1)SortedNodes:		an array that contains node's information that are in topological order
	//2)DFGSize:			size of input DFG
	//----------------------------------------------------------------------------------------------
	
	//definition of variables------------------------------------
	int Ordering = 0; //order nodes from output to input
	unsigned int i, j;//tempelate variables
	bool conditionOutput;
	int tempNode = 0;
	vector<int> nodeIndexes;//vector array that stores all nodes
	//-----------------------------------------------------------

	//initial condition that puts all traceFlag to false
	for(int iTrace = 0; iTrace < DFGSize; iTrace++)
		inputNodes[iTrace].traceFlag = false;
	//--------------------------------------------------



	//insert nodes that have no output------------------
	for(i = 0; i < DFGSize; i++)
		if(inputNodes[i]._NumberOfOutputs == 0)
			nodeIndexes.push_back(i);
	//--------------------------------------------------

	

	//basic part of algorithm-loop until all nodes trace--------------------------
	while(!nodeIndexes.empty())
	{//pop items from nodeIndexes
		conditionOutput = true;
		tempNode = nodeIndexes.front();//pop the first item in nodeIndexes vector
		nodeIndexes.erase(nodeIndexes.begin());//erease this item from vector

		//checks if all outputs of this node traced before---------------------
		for(i = 0; i < inputNodes[tempNode]._Output.size(); i++)
		{
			if(inputNodes[inputNodes[tempNode]._Output[i]].traceFlag == false)
				conditionOutput = false;
		}
		//---------------------------------------------------------------------

		//add parents of selected nodes to nodeIndexes vector---------------
		if(conditionOutput && inputNodes[tempNode].traceFlag == false)
		{//add parents to nodeIndex
			inputNodes[tempNode]._Order = Ordering;
			inputNodes[tempNode].traceFlag = true;
			Ordering++;

			for(j = 0; j < inputNodes[tempNode]._NumParents; j++)
				nodeIndexes.push_back(inputNodes[tempNode]._Parents[j]);
		}
		//------------------------------------------------------------------
	}
}
//------------------------------------------------------------------------------------------------------
void topologicalSortDFG(ExprNode* SortedNodes, ExprNode* Nodes, int DFGSize)
{//begin of topologicalSortDFG
	//1)SortedNodes:		an array that contains node's information that are in topological order
	//2)Nodes:				an array that contains node's information
	//3)DFGSize:			size of input DFG
	//----------------------------------------------------------------------------------------------
	
	//template variables
	int i, j, k;
	//------------------

	//first copy all nodes from nodes to sortednodes
	for(i = 0; i < DFGSize; i++)
		for(j = 0; j < DFGSize; j++)
			if(Nodes[j]._Order == i)
				SortedNodes[i] = Nodes[j];
	//----------------------------------------------

	//change the order of output and parents nodes based on previous order
	for(i = 0; i < DFGSize; i++)
	{
		//order of output nodes----------------------------------
		for(j = 0; j < SortedNodes[i]._NumberOfOutputs; j++)
		{
			for(k = 0; k < DFGSize; k++)
				if(SortedNodes[i]._Output[j] == k)
				{
					SortedNodes[i]._Output[j] = Nodes[k]._Order;
					break;
				}
		}
		//-------------------------------------------------------

		//order of parent nodes----------------------------------
		for(j = 0; j < SortedNodes[i]._NumParents; j++)
			for(k = 0; k < DFGSize; k++)
				if(SortedNodes[i]._Parents[j] == k)
				{
					SortedNodes[i]._Parents[j] = Nodes[k]._Order;
					break;
				}
		//-------------------------------------------------------
	}
	//---------------------------------------------------------------------
}

//------------------------------------------------------------------------------------------------------
vector<int> numberParents(CI* inCI, int nodeNum, ExprNode* SortedNodes)
{//begin of numberParents(return a vector of parents that are in CIs)
	//1)inCI:				the input CI
	//2)nodeNum:			takes the node number
	//3)SortedNodes:		an array that contains node's information that are in topological order
	//----------------------------------------------------------------------------------------------
	
	//template variable
	int i, j;
	//-----------------

	//result vector----
	vector<int> result;
	//-----------------

	//loop over all of parents-------------------------------------
	for(i = 0; i < SortedNodes[nodeNum]._NumParents; i++)
	{
		//check if the parent of nodeNum are in CI
		for(j = 0; j < inCI->Nodes.size(); j++)
			if(inCI->Nodes[j] == SortedNodes[nodeNum]._Parents[i])
				result.push_back(SortedNodes[nodeNum]._Parents[i]);
	}
	//-------------------------------------------------------------

	return result;
}
//------------------------------------------------------------------------------------------------------
void checkConstant(ExprNode* SortedNodes, int DFGSize)
{//begin of checkConstant
	//1)SortedNodes:		an array that contains node's information that are in topological order
	//2)DFGSize:			size of input DFG
	//----------------------------------------------------------------------------------------------
	
	//definittion of variables-------------------
	bool hasCons = false;
	int i, j;
	bool testCons[1000];
	memset(testCons, false, 1000 * sizeof(bool));
	//-------------------------------------------

	//loop over all nodes--------------------------------------------------------------------
	//1)if it has a constant as parent, make a flag true
	//2)if it is SHL/SHR make hwlatency to zero
	for(i = 0; i < DFGSize; i++)
	{
		for(j = 0; j < SortedNodes[i]._Parents.size(); j++)
		{
			if(SortedNodes[i]._Parents[j] < 0) 
			{
				SortedNodes[i]._Parents[j]=0;
				SortedNodes[i]._NumParents--;
				//break;
			}
			if(!SortedNodes[SortedNodes[i]._Parents[j]]._Op.compare("CONS"))
			{
				hasCons = true;
				SortedNodes[i].hasCons = true;
				testCons[i] = true;
				//checks for SHL/SHR--------------------------------------------------------
				if(!SortedNodes[i]._Op.compare("SHL") || !SortedNodes[i]._Op.compare("SHR"))
					SortedNodes[i]._HwTime = 0;
				//--------------------------------------------------------------------------
				break;
			}
		}
		if(testCons[i] == false) //SortedNodes[i]._HwTime != 0
			SortedNodes[i].hasCons = false;
	}
	//---------------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------------------------------
TInOutNum findPermanentInputOutput(ExprNode* SortedNodes, bool* CIcand, int DFGSize, int CurrentLevel, int& PermanentInputsNum)
{//begin of findPermanentInputOutput
	//1)SortedNodes:		an array that contains node's information that are in topological order
	//2)CIcand:				a boolean array that shows the custom instruction
	//3)DFGSize:			size of input DFG
	//4)CurrentLevel:		the last node number that are checked
	//5)PermanentInputsNum	it stores the number of permanent inputs(it consider as one of the output of the function)
	//----------------------------------------------------------------------------------------------------------------
	
	//variable declaration-------------------------------------------------
	int i, j, k; //template variable
	TInOutNum Tcut;
	Tcut.inNum = Tcut.outNum = 0;
	bool Input_to_cut_Flag = false; //checks if inputs were counted before
	int* input_to_cut = new int[DFGSize + 1];//an array of inputs to CI
	memset(input_to_cut, 0, (DFGSize + 1) * sizeof(int));
	int len_input_to_cut = 0; //counter for input_to_cut array
	//---------------------------------------------------------------------

	for(i = 0; i < DFGSize; i++)
	{//search in all of nodes
		if(CIcand[i] == 1)//true if this node is in CI
		{
			for(j = 0; j < SortedNodes[i]._NumParents; j++)//count inputs and permanent input
			{
				if(CIcand[SortedNodes[i]._Parents[j]] == 0)
				{
					//checks parents of CI---------------------------------------------
					Input_to_cut_Flag = false;
					for(k = 0; k < len_input_to_cut; k++)
					{
						if(input_to_cut[k] == SortedNodes[i]._Parents[j])
						{
							Input_to_cut_Flag = true;
							break;
						}
					}
					//------------------------------------------------------------------
	
					//add inputs to input_to_cut----------------------------------------
					if(!Input_to_cut_Flag) 
					{
						input_to_cut[len_input_to_cut] = SortedNodes[i]._Parents[j];
						len_input_to_cut++;
						if(SortedNodes[SortedNodes[i]._Parents[j]]._Op.compare("CONS"))
							Tcut.inNum++;
					}
					//-------------------------------------------------------------------

					//count number of permanent inputs----------------------------------------------------------------------
					if((SortedNodes[SortedNodes[i]._Parents[j]]._ForbiddenNode || SortedNodes[i]._Parents[j] < CurrentLevel) 
								&& (!Input_to_cut_Flag))
									if(SortedNodes[SortedNodes[i]._Parents[j]]._Op.compare("CONS"))
											PermanentInputsNum++;
					//------------------------------------------------------------------------------------------------------
				}
			}

			//count output number of CI-------------
			if(SortedNodes[i]._NumberOfOutputs == 0)
					Tcut.outNum++;
			//--------------------------------------

			//count other output of custom instructions-------------------------------------------
			bool searchChild = false;
			for(k = 0; k < SortedNodes[i]._NumberOfOutputs; k++)
				if(CIcand[SortedNodes[i]._Output[k]] == 0) //true if output of CI is not in CI
				{
					searchChild = true;
					break;
				}
			if(searchChild && !(SortedNodes[i]._NumberOfOutputs == 0))
				Tcut.outNum++;
			//------------------------------------------------------------------------------------
		}
	}

	return Tcut;
}

//--------------------------------------------------------------------------------------
bool checkParent(bool* CIcand,CI* basic,int numbasic, CI* tmp, int numtmp,ExprNode* SortedNodes)
{//begin of checkParent
	//1)CIcand:				a boolean array that shows the custom instruction
	//2)basic:				one of the CI
	//3)numbasic:			the number of node that will be checked
	//4)tmp:				the other CI
	//5)numtmp:				the number of node that will be checked
	//6)SortedNodes:		an array that contains node's information that are in topological order
	//---------------------------------------------------------------------------------------------
	
	//call to function numberParents to get the parents nodes
	vector<int> basicParent, tmpParent;
	basicParent = numberParents(basic, numbasic, SortedNodes);
	tmpParent = numberParents(tmp, numtmp, SortedNodes);
	//-------------------------------------------------------
    
	//definition of variables-----------------------------
	bool child_eq = true;
	bool* basPar = new bool[basicParent.size()];
	bool* tmpPar = new bool[basicParent.size()];
	memset(basPar, 0 , basicParent.size() * sizeof(bool));
	memset(tmpPar, 0 , basicParent.size() * sizeof(bool));
	//----------------------------------------------------

	//escape condition of recursive function
	if(basic == 0 && tmp == 0)
		return true;
	else if (basic == 0 && tmp != 0)
		return false;
	else if (basic != 0 && tmp == 0)
		return false;
	//checks over other information on CIs------
	else if (basic->outputNum != tmp->outputNum)
	{
		delete[] basPar;
		basPar = NULL;
		delete[] tmpPar;
		tmpPar = NULL;
		return false;
	}
	else if (basic->Nodes.size() != tmp->Nodes.size())
	{
		delete[] basPar;
		basPar = NULL;
		delete[] tmpPar;
		tmpPar = NULL;
		return false;
	}
	//------------------------------------------

	



	//check the equality of node for the operations that has commutativity---------------------------------------------
	else if (SortedNodes[numbasic]._Op.compare(SortedNodes[numtmp]._Op) == 0)
	{
		
		if (SortedNodes[numbasic]._Op.compare("ADD") == 0 ||
			SortedNodes[numbasic]._Op.compare("MUL") == 0 ||
			SortedNodes[numbasic]._Op.compare("AND") == 0 ||
			SortedNodes[numbasic]._Op.compare("OR") == 0 ||
			SortedNodes[numbasic]._Op.compare("NOR") == 0 ||
			SortedNodes[numbasic]._Op.compare("XNOR") == 0 ||
			SortedNodes[numbasic]._Op.compare("NAND") == 0 ||
			SortedNodes[numbasic]._Op.compare("XOR") == 0)
		{//check commutative operations
			if(basicParent.size() == tmpParent.size())
			{//checks size of CIs
				if(basicParent.size() > 0)
					for(int ii=0; ii < basicParent.size(); ii++)
					{//loop on basicParent
						for(int jj=0; jj < tmpParent.size(); jj++)
						{//loop on tmpParent
						
							if (basPar[ii] == false &&
								tmpPar[jj] == false &&
								checkParent(CIcand, basic, basicParent[ii], tmp, tmpParent[jj], SortedNodes)) //calls recursive function
								{
									if( ii != jj)
										changeable++;
									basPar[ii] = true;
									tmpPar[jj] = true;
								}//end of if
						}
					}
				else
				{
					delete[] basPar;
					basPar = NULL;
					delete[] tmpPar;
					tmpPar = NULL;
					return true;
				}
			}
			else
			{
				delete[] basPar;
				basPar = NULL;
				delete[] tmpPar;
				tmpPar = NULL;
				return false; //return false if the size of two CI are not equal
			}
		}
		//------------------------------------------------------------------------------------------------------------


		else //for operation that are not changeable
		{
			if(basicParent.size() == tmpParent.size())
			{
				if(basicParent.size() > 0)
					for(int ii=0; ii < basicParent.size(); ii++)
					{
					//if(CIcand[tmp->_Parents[ii]] && CIcand[basic->_Parents[ii]])
						if (basPar[ii] == false &&
							tmpPar[ii] == false &&
							checkParent(CIcand, basic, basicParent[ii], tmp, tmpParent[ii], SortedNodes))
						{
							basPar[ii] = true;
							tmpPar[ii] = true;
						}//end of if
					}//end of first loop
				else
				{
					delete[] basPar;
					basPar = NULL;
					delete[] tmpPar;
					tmpPar = NULL;
					return true;
				}
			}//end of if
			else
			{
				delete[] basPar;
				basPar = NULL;
				delete[] tmpPar;
				tmpPar = NULL;
				return false;
			}
		}//end of else

		for( int cii = 0; cii < basicParent.size(); cii++)
			if(basPar[cii] == false)
				child_eq = false;
		if(child_eq == true)
		{
			delete[] basPar;
			basPar = NULL;
			delete[] tmpPar;
			tmpPar = NULL;
			return true;
		}
		else
		{
			delete[] basPar;
			basPar = NULL;
			delete[] tmpPar;
			tmpPar = NULL;
			return false;
		}
	}//end of if that checks equality of operation
	else
	{
		delete[] basPar;
		basPar = NULL;
		delete[] tmpPar;
		tmpPar = NULL;
		return false;
		}
}
//----------------------------------------------------------------------------------------
bool checkReconfigurable(bool* CIcand,CI* basic,int numbasic, CI* tmp, int numtmp,ExprNode* SortedNodes)
{//begin of checkParent
	//1)CIcand:				a boolean array that shows the custom instruction
	//2)basic:				one of the CI
	//3)numbasic:			the number of node that will be checked
	//4)tmp:				the other CI
	//5)numtmp:				the number of node that will be checked
	//6)SortedNodes:		an array that contains node's information that are in topological order
	//---------------------------------------------------------------------------------------------
	
	//call to function numberParents to get the parents nodes
	vector<int> basicParent, tmpParent;
	basicParent = numberParents(basic, numbasic, SortedNodes);
	tmpParent = numberParents(tmp, numtmp, SortedNodes);
	//-------------------------------------------------------
    
	//definition of variables-----------------------------
	bool child_eq = true;
	bool* basPar = new bool[basicParent.size()];
	bool* tmpPar = new bool[basicParent.size()];
	memset(basPar, 0 , basicParent.size() * sizeof(bool));
	memset(tmpPar, 0 , basicParent.size() * sizeof(bool));
	//----------------------------------------------------

	//escape condition of recursive function
	if(basic == 0 && tmp == 0)
		return true;
	else if (basic == 0 && tmp != 0)
		return false;
	else if (basic != 0 && tmp == 0)
		return false;
	//checks over other information on CIs------
	else if (basic->outputNum != tmp->outputNum)
	{
		delete[] basPar;
		basPar = NULL;
		delete[] tmpPar;
		tmpPar = NULL;
		return false;
	}
	else if (basic->Nodes.size() != tmp->Nodes.size())
	{
		delete[] basPar;
		basPar = NULL;
		delete[] tmpPar;
		tmpPar = NULL;
		return false;
	}
	//------------------------------------------

	



	//check the equality of node for the operations that has commutativity---------------------------------------------
	else if (SortedNodes[numbasic]._Op.compare(SortedNodes[numtmp]._Op) == 0)
	{
		
		if (SortedNodes[numbasic]._Op.compare("ADD") == 0 ||
			SortedNodes[numbasic]._Op.compare("MUL") == 0 ||
			SortedNodes[numbasic]._Op.compare("AND") == 0 ||
			SortedNodes[numbasic]._Op.compare("OR") == 0 ||
			SortedNodes[numbasic]._Op.compare("NOR") == 0 ||
			SortedNodes[numbasic]._Op.compare("XNOR") == 0 ||
			SortedNodes[numbasic]._Op.compare("NAND") == 0 ||
			SortedNodes[numbasic]._Op.compare("XOR") == 0)
		{//check commutative operations
			if(basicParent.size() == tmpParent.size())
			{//checks size of CIs
				if(basicParent.size() > 0)
					for(int ii=0; ii < basicParent.size(); ii++)
					{//loop on basicParent
						for(int jj=0; jj < tmpParent.size(); jj++)
						{//loop on tmpParent
						
							if (basPar[ii] == false &&
								tmpPar[jj] == false &&
								checkParent(CIcand, basic, basicParent[ii], tmp, tmpParent[jj], SortedNodes)) //calls recursive function
								{
									if( ii != jj)
										changeable++;
									basPar[ii] = true;
									tmpPar[jj] = true;
								}//end of if
						}
					}
				else
				{
					delete[] basPar;
					basPar = NULL;
					delete[] tmpPar;
					tmpPar = NULL;
					return true;
				}
			}
			else
			{
				delete[] basPar;
				basPar = NULL;
				delete[] tmpPar;
				tmpPar = NULL;
				return false; //return false if the size of two CI are not equal
			}
		}
		//------------------------------------------------------------------------------------------------------------


		else //for operation that are not changeable
		{
			if(basicParent.size() == tmpParent.size())
			{
				if(basicParent.size() > 0)
					for(int ii=0; ii < basicParent.size(); ii++)
					{
					//if(CIcand[tmp->_Parents[ii]] && CIcand[basic->_Parents[ii]])
						if (basPar[ii] == false &&
							tmpPar[ii] == false &&
							checkParent(CIcand, basic, basicParent[ii], tmp, tmpParent[ii], SortedNodes))
						{
							basPar[ii] = true;
							tmpPar[ii] = true;
						}//end of if
					}//end of first loop
				else
				{
					delete[] basPar;
					basPar = NULL;
					delete[] tmpPar;
					tmpPar = NULL;
					return true;
				}
			}//end of if
			else
			{
				delete[] basPar;
				basPar = NULL;
				delete[] tmpPar;
				tmpPar = NULL;
				return false;
			}
		}//end of else

		for( int cii = 0; cii < basicParent.size(); cii++)
			if(basPar[cii] == false)
				child_eq = false;
		if(child_eq == true)
		{
			delete[] basPar;
			basPar = NULL;
			delete[] tmpPar;
			tmpPar = NULL;
			return true;
		}
		else
		{
			delete[] basPar;
			basPar = NULL;
			delete[] tmpPar;
			tmpPar = NULL;
			return false;
		}
	}//end of if that checks equality of operation
	else
	{
		delete[] basPar;
		basPar = NULL;
		delete[] tmpPar;
		tmpPar = NULL;
		return false;
		}
}
//----------------------------------------------------------------------------------------
bool CheckConvex(bool* CIcand, int** TransitionMatrix, int NodeNum )
{//begin of CheckConvex(i.e.This is convex, i.e., if there exists no path from 
	//a node that is member of cut to another node that is also membear of cut
	//through a node that is not a member of cut)
	//1)CIcand:				a boolean array that shows the custom instruction
	//2)TransitionMatrix	transition matrix that stores the structure of the tree
	//3)NodeNum				size of input DFG
	//---------------------------------------------------------------------------------------------
	
	//definition of basic variables and initialization-----------
	bool isconvex = true;
	int* in_cut = new int[NodeNum];
	memset(in_cut, 0, NodeNum * sizeof(int));
	int len_in_cut = 0;
	int* fifo = new int[NodeNum];
	int** X = new int*[NodeNum];
	for(int w = 0; w < NodeNum; w++)
		X[w] = new int[NodeNum];
	int* out_cut = new int[NodeNum];
	memset(out_cut, 0, NodeNum * sizeof(int));
	int len_out_cut = 0;
	memset(fifo, 0, NodeNum * sizeof(int));
	int len_fifo = -1;
	bool all_zero_flag;
	//-----------------------------------------------------------

	//template variables
	int k01, k02, k03, k04;
	int i, j;
	//------------------

	//loop over all nodes and fill in_cut and out_cut----------------
	for ( i=0; i< NodeNum; i++)
		if (CIcand[i] == 1)
		{
			in_cut[len_in_cut] = i; //the nodes that are in cut
			len_in_cut++;
		}
		else
		{
			out_cut[len_out_cut] = i; //the nodes that are not in cut
			len_out_cut++;
		}
	//---------------------------------------------------------------
	

	//fill X with transition matrix----------
	for ( i=0; i< NodeNum; i++)
		for ( j=0; j< NodeNum; j++)
			X[i][j] = TransitionMatrix[i][j];
	//---------------------------------------

	for (k01 = 0; k01< len_in_cut; k01++)
	{//loop over in_cut
		for (k02 = 0; k02< len_out_cut; k02++)
		{//loop over out_cut
			if (X[in_cut[k01]][out_cut[k02]])
			{//check edge between these nodes in X
				k03 = out_cut[k02];
				while(1)
				{
					all_zero_flag = true;
					for (k04 = 0; k04< NodeNum; k04++)
					{
						if (X[k03][k04])
						{//an edge between nodes that one of them are in cut and the other one is not
							for (int i=0; i< len_in_cut; i++)
							{
								if ( in_cut[i] == k04 ) 
								{
									isconvex = false;

									//delete unused array---------------
									for (int i = 0; i < NodeNum; i++) {
										delete[] X[i];
											X[i] = NULL;
									}
									delete[] X;
									X = NULL;
									delete[] fifo;
									fifo = NULL;
									delete[] in_cut;
									in_cut = NULL;
									delete[] out_cut;
									out_cut = NULL;
									//----------------------------------
									return isconvex;
								}
							}
							len_fifo++;
							fifo[len_fifo] = k03;
							k03 = k04;
							all_zero_flag = false;
							break;
						}
					}
	                
					if (all_zero_flag)
					{
						if (len_fifo == -1)
							break;
						else
						{
							for(int i=0; i< NodeNum; i++)
								X[i][k03] = 0;

							k03 = fifo[len_fifo];
							len_fifo--;
						}	
					}
				}
			}
		}
	}

	//delete unused array---------------
	for (int i = 0; i < NodeNum; i++) {
		delete[] X[i];
		X[i] = NULL;
	}
	delete[] X;
	X = NULL;
	delete[] fifo;
	fifo = NULL;
	delete[] in_cut;
	in_cut = NULL;
	delete[] out_cut;
	out_cut = NULL;
	//---------------------------------
	
	return isconvex;
	
}
//-------------------------------------------------------------------------------------------------
bool checkSimilarityCI(bool* CIcand, CI* temp, ExprNode* SortedNodes, int lenCI, int& intFind)
{//begin of checkSimilarityCI(check similarity between new CI and previously stores CI)
	//1)CIcand:				a boolean array that shows the custom instruction
	//2)temp:				new CI
	//3)SortedNodes:		an array that contains node's information that are in topological order		
	//4)lenCI:				the number of CI that until now found
	//5)intFind:			return the id of similliar node
	//---------------------------------------------------------------------------------------------
	
	//variable definition
	bool breakL = false;
	int findCI = 0;
	int jj, parI;
	int rootI, rootJ;
	//------------------

	//outer loop to circulate in CIarray--------------------------------------------------------
	//for(jj = 0; jj < CIarray.size(); jj++)
	for(jj = 0; jj < lenCI; jj++)
	{
		//new arrays that stores roots of custom instruction
		bool* checkPar = new bool[temp->roots.size()];
		memset(checkPar, 0, temp->roots.size() * sizeof(bool));

		bool* tmpPar = new bool[temp->roots.size()];
		memset(tmpPar, 0, temp->roots.size() * sizeof(bool));
		//---------------------------------------------------

		breakL = false;
		findCI = 0;

		//checks the information of custom instruction
		if( temp->outputNum    == CIarray[jj]->outputNum &&
			temp->HWLatency   == CIarray[jj]->HWLatency  &&
			temp->numNodes     == CIarray[jj]->numNodes  &&
			temp->roots.size() == CIarray[jj]->roots.size())
		{
			for(rootI = 0; rootI < CIarray[jj]->roots.size(); rootI++)
				for(rootJ = 0; rootJ < temp->roots.size(); rootJ++)
				{
					if(tmpPar[rootJ] == false)
						if(checkParent(CIcand, CIarray[jj], CIarray[jj]->roots[rootI],temp, temp->roots[rootJ], SortedNodes)) //should change
						{
							checkPar[rootI] = true;
							tmpPar[rootJ] = true;
							break;
						}
				}
		}

		for(parI = 0; parI < temp->roots.size(); parI++)
			if(!checkPar[parI])
				breakL = true;
		if(!breakL)//breakL is false if it finds a similliar node
		{
			findCI = jj;
			intFind = findCI;
			break;
		}
		delete[] checkPar;
		checkPar = NULL;
		delete[] tmpPar;
		tmpPar = NULL;
	}

	if(breakL || lenCI == 0)
		return false;//return false if it didn't find any similliar node or it is the first CI
	else
		return true;//return true if it found a similliar node
}
//-------------------------------------------------------------------------------------------------
void updateCI(bool* CIcand, ExprNode* SortedNodes, int DFGSize, TInOutNum Tcut, int& lenCI)
{//begin of updateCI(update the CIarray)
	//1)CIcand:				a boolean array that shows the custom instruction
	//2)SortedNodes:		an array that contains node's information that are in topological order		
	//3)DFGSize:			size of input DFG
	//4)Tcut:				input and output port number
	//5)lenCI:				it stores information on number of CIs that until now founded
	//---------------------------------------------------------------------------------------------
	
	//variable definition-------------------------------
	CI* temp = new CI();//it stores the newly founded CI
	int iterationRep = 0;
	int i, j;//template variable
	int findCI;
	double oldMerit, newMerit, HardwareLatency;
	//--------------------------------------------------

	//call to merit functions------------------------------------------------
	oldMerit = oldMeritValue(CIcand, SortedNodes, DFGSize);
	newMerit = FindMeritValue(CIcand, SortedNodes, DFGSize, HardwareLatency);
	//-----------------------------------------------------------------------

	//initialization of variables
	temp->numNodes = 0;
	temp->NumRep = 1;
	double tmpArea = 0;
	//Mehdi Kamal Power
	double tmpPower = 0;
	bool immediateValue = false;
	for(i = 0; i < DFGSize; i++)
	{
		if(CIcand[i])
		{
			immediateValue = false;
			for(j =0; j < SortedNodes[i]._Parents.size(); j++)
			{
				if(SortedNodes[SortedNodes[i]._Parents[j]]._Op.compare("CONS") == 0)
				{
					immediateValue = true;
					break;
				}
			}
			tmpPower += (!immediateValue)? SortedNodes[i]._DynamicPower : SortedNodes[i]._immediateDynamicPower;
		}
	}


	//---------------------------

	//loop over all DFG nodes and sum the areas
	for(i = 0; i < DFGSize; i++)
	{
		if(CIcand[i])
		{
			tmpArea += SortedNodes[i]._Area;
		}
	}
	//-----------------------------------------

	//loop over all DFG nodes and get the iteration number
	for(i = 0; i < DFGSize; i++)
	{
		if(CIcand[i])
		{
			iterationRep = SortedNodes[i].iteration;
			break;
		}
	}
	//----------------------------------------------------

	//update all information in CI
	//merit information-----------------------
	temp->merit = newMerit * iterationRep; 
	temp->selfMerit = newMerit * iterationRep;
	temp->oldMerit = oldMerit * iterationRep;
	//----------------------------------------
	temp->area = tmpArea;//area
	temp->power = tmpPower;
	temp->HWLatency = HardwareLatency;//HWLatency
	temp->Iteration = iterationRep;//iteration number
	temp->inputNum = Tcut.inNum;//input port number
	//output port number---------------------------
	if(Tcut.outNum == 0)
		temp->outputNum = 1;
	else
		temp->outputNum = Tcut.outNum;
	//---------------------------------------------
	
	//find roots and other nodes of CI-----------------------------------------------------
	for(i = 0; i < DFGSize; i++)  
	{//loop over all nodes that are in CI
		if(CIcand[i])
		{
			
			temp->Nodes.push_back(i);//add nodes to Nodes vector
			temp->numNodes++;//increment numNodes by one
			if(SortedNodes[i]._Output.empty())//add nodes to root that has no output ports
				temp->roots.push_back(i);
			else
			{//add node to root that has output but not in CI 
				bool flagOut = false;
				for(j=0; j < SortedNodes[i]._NumberOfOutputs; j++)
					if(CIcand[SortedNodes[i]._Output[j]])
					{
						flagOut =true;
						break;
					}
				if(!flagOut)
				{
					if(SortedNodes[i]._NumberOfOutputs != 0)
					{
						temp->roots.push_back(i);
					}
				}
			}
		}
	}
	//---------------------------------------------------------------------------------------
	
	//checks if CI has any similarity in CIarray
	if(checkSimilarityCI(CIcand, temp, SortedNodes, lenCI, findCI))
	{
		CIarray[findCI]->merit += temp->merit; //add this merit to previously stored CI
		CIarray[findCI]->otherMerit.push_back(temp->merit);//push other merit to otherMerit vector
		CIarray[findCI]->oldotherMerit.push_back(temp->oldMerit);//push other old merit to oldotherMerit vector
		CIarray[findCI]->otherIteration.push_back(temp->Iteration);
		CIarray[findCI]->NumRep++;//increment number of Repitation
		//push other operation in otherOp array-----------------
		for(int iR = 0; iR < temp->numNodes; iR++)
			CIarray[findCI]->otherOp.push_back(temp->Nodes[iR]);
		//------------------------------------------------------
		temp->Nodes.clear();
		temp->roots.clear();
		temp->otherOp.clear();
		temp->otherMerit.clear();
		delete temp;
		temp = NULL;
	}
	else
	{
		CIarray[lenCI] = new CI();
		CIarray[lenCI] = temp;
		//CIarray.push_back(temp);
		lenCI++;
	}
}
//------------------------------------------------------------------------------------------------------------------
int findLevelofTree(ExprNode* Nodes, int DFGSize)
{//begin of findLevelofTree
	//1)Nodes:				an array that contains node's information		
	//2)DFGSize:			size of input DFG
	//---------------------------------------------------------------------------------------------
	
	//variable definition-------
	vector<ExprNode*> findLevel;
	ExprNode* tempLevel;
	int finalLevel = 0;
	//--------------------------

	//loop over DFG and put nodes that have no output in findLevel vector
	for(int level = 0; level < DFGSize; level++)
	{
		if(Nodes[level]._NumberOfOutputs == 0)
		{
			Nodes[level].level = 0;
			Nodes[level].flagLevel = true;
			findLevel.push_back(&Nodes[level]);
		}
	}
	//-------------------------------------------------------------------

	//loop until all nodes are traversed
	while(!findLevel.empty())
	{
		tempLevel = findLevel.front();
		for(int iParents = 0; iParents < tempLevel->_NumParents; iParents++) //checks number of parents
		{
			if(Nodes[tempLevel->_Parents[iParents]]._Op.compare("IN") != 0 && Nodes[tempLevel->_Parents[iParents]]._Op.compare("CONS") != 0)
			{
				if(Nodes[tempLevel->_Parents[iParents]].flagLevel = true && Nodes[tempLevel->_Parents[iParents]].level < tempLevel->level + 1)
				{
					findLevel.push_back(&Nodes[tempLevel->_Parents[iParents]]);
					Nodes[tempLevel->_Parents[iParents]].level = tempLevel->level + 1;
				}
			}
		}
		findLevel.erase(findLevel.begin());
	}

	//loop over all nodes to find the critical path of DFG
	for(int fL = 0; fL < DFGSize; fL++)
	{
		if(Nodes[fL].level > finalLevel)
			finalLevel = Nodes[fL].level;
	}
	//-----------------------------------------------------

	return finalLevel;
}
//--------------------------------------------------------------------------------------------------------------------
bool boolInCI(conflictNode* in, int verilogNode)
{//begin of boolInCI(it will be used in generation of dotty file)
	//1)in:					conflict node that are many nodes in it
	//2)verilogNode:		id of a verilog node
	//-------------------------------------------------------------
	
	//template variable
	int i;
	//-----------------

	//loop over all nodes----------------
	for(i = 0; i < in->Nodes.size(); i++)
	{
		if(in->Nodes[i] == verilogNode)
			return true;
	}
	//-----------------------------------
	return false;
}
//-----------------------------------------------------------------------------------------------------------------------
void UpdateVector(vector<int>& verilogInputs,int intNode)
{//begin of UpdateVector(it will be used in generation of dotty file)
	//1)verilogInputs:		a vector that contains unique nodes in it
	//2)intNode:			id of a verilog node
	//-------------------------------------------------------------
	
	//variable definition
	bool found = false;
	//-------------------

	//loop over verilogInput nodes to check if already added or not
	for(int i = 0;  i < verilogInputs.size(); i ++)
	{
		if(verilogInputs[i] == intNode)
			found = true;
	}
	//-------------------------------------------------------------
	
	if(found == false)
		verilogInputs.push_back(intNode);

}
//------------------------------------------------------------------------------------------------------------------------
bool isInConflictNode(conflictNode* inNode ,int inputNode)
{//begin of isInConflictNode(it will be used in generation of dotty file)
	//1)verilogInputs:		a vector that contains unique nodes in it
	//2)intNode:			id of a verilog node
	//-------------------------------------------------------------
	
	//variable definition
	bool found = true;
	//-------------------

	//loop over all nodes that are in inNode-----
	for(int i = 0; i < inNode->Nodes.size(); i++)
		if(inNode->Nodes[i] == inputNode)
		{
			found = false;
			break;
		}	
	//-------------------------------------------
	return found;//true if not found, false if found
}
//------------------------------------------------------------------------------------------------------------------------
void findVerilogInputs(conflictNode* inNode, vector<int>& verilogInputs, ExprNode* SortedNodes)
{//begin of findVerilogInputs(it will be used in generation of dotty file)
	//1)inNode:					a conflict node
	//2)verilogInputs:			a vector that has all inputs of a conflictNode
	//3)SortedNodes:			an array that contains node's information that are in topological order	
	//-------------------------------------------------------------
	
	//template variable
	int i, j = 0;
	//-----------------

	//loop over nodes that are in inNode----------------------------------------------
	for(i = 0; i < inNode->Nodes.size(); i++)
	{
		for(j = 0; j < SortedNodes[inNode->Nodes[i]]._Parents.size(); j++)
		{
			if(isInConflictNode(inNode,SortedNodes[inNode->Nodes[i]]._Parents[j]))
				UpdateVector(verilogInputs,SortedNodes[inNode->Nodes[i]]._Parents[j]);
		}
	}
	//---------------------------------------------------------------------------------
}
//-------------------------------------------------------------------------------------------------------------------------
void findVerilogOutputs(conflictNode* inNode, vector<int>& verilogOutputs, ExprNode* SortedNodes)
{//begin of findVerilogOutputs(it will be used in generation of dotty file)
	//1)inNode:					a conflict node
	//2)verilogInputs:			a vector that has all inputs of a conflictNode
	//3)SortedNodes:			an array that contains node's information that are in topological order	
	//-------------------------------------------------------------
	
	//template variable
	bool found;
	int i, j, k;
	//-----------------

	//loop over all nodes that are in inNode---------------------------------------
	for(i = 0; i < inNode->Nodes.size(); i++)
	{
		found = true;
		for(j = 0; j < SortedNodes[inNode->Nodes[i]]._Output.size(); j++)
			for(k = 0; k < inNode->Nodes.size(); k++)
				if(SortedNodes[inNode->Nodes[i]]._Output[j] == inNode->Nodes[k])
					found = false;

		if(found)
			verilogOutputs.push_back(inNode->Nodes[i]);//adds nodes to verilogOutputs if the number of output ports are zero or is not in inNode
	}
	//---------------------------------------------------------------------------
}
//------------------------------------------------------------------------------------------------------------------------------
int printDotty(conflictNode* inNode, int verilogInputs,std::ofstream& verilogCI, ExprNode* SortedNodes)
{//begin of printDotty(it will be used in generation of dotty file-recursive function)
	//1)inNode:					conflict nodes
	//2)verilogInputs:			a vector that has all inputs of a conflictNode
	//3)verilogCI:				ofstream
	//4)SortedNodes:			an array that contains node's information that are in topological order	
	//-------------------------------------------------------------------------------------------------

	if(SortedNodes[verilogInputs]._Output.size() == 0 || SortedNodes[verilogInputs].isTraced == true )
		return 0;
	for(int i = 0; i < SortedNodes[verilogInputs]._Output.size(); i++)
		if(!isInConflictNode(inNode, SortedNodes[verilogInputs]._Output[i]))
		{
			verilogCI << SortedNodes[verilogInputs].lastID << " -> " << SortedNodes[SortedNodes[verilogInputs]._Output[i]].lastID << ";\n";

			printDotty(inNode, SortedNodes[verilogInputs]._Output[i], verilogCI, SortedNodes);
			if(SortedNodes[verilogInputs]._Output.size() == i + 1)
			{
				SortedNodes[verilogInputs].isTraced = true;
				return 0;
			}
		}
}
//------------------------------------------------------------------------------------------------------------------
void printResult(string basicFile, int conflictNumber, int DFGSize, ExprNode* SortedNodes, ExprNode* Nodes, 
				 conflictNode* conflictArray, TInOutNum T, precision_timer selectTimer, int objectiveFunction, bool localAlgorithm, int inRegisterConstraint
				 , int outRegisterConstraint, bool firstIteration)
{//begin of function
	ofstream fileCI;
	size_t found, mkdirFound;
	found = basicFile.find("txt");
	char buffer[200];

	ofstream fileNodes;
	char allNode[200];
	string finalResult;
	string strMkdir;
	char charObj[3];
	cout << "begining of the printResult\n";

	found = basicFile.find_last_of("\\");
	finalResult = basicFile;
	basicFile.erase(found + 1, basicFile.size() - found - 1);
	basicFile.insert(basicFile.size(), "allnodes.txt");

	size_t finalakhar, finalfilelen;
	finalfilelen = basicFile.size();
	finalakhar = basicFile.copy(allNode, finalfilelen, 0);
	allNode[finalakhar] = '\0';

	fileNodes.open(allNode);
	fileNodes << "NodeID\t" << "Output\t" << "SortedNodeID"<<"\n";
	for(int yy = 0; yy < DFGSize; yy++)
	{
		fileNodes << SortedNodes[yy].lastID << "\t";
		for(int parents = 0; parents < SortedNodes[yy]._NumberOfOutputs; parents++)
			fileNodes << SortedNodes[SortedNodes[yy]._Output[parents]].lastID - 1 << ";";
		if(SortedNodes[yy]._NumberOfOutputs == 0)
			fileNodes << "\t" << 0 << "\t";
		fileNodes << "\t" << yy << "\t" << endl;
	}

	int levelOfTree;
	levelOfTree = findLevelofTree(Nodes, DFGSize);

	size_t finalFound;
	finalFound = finalResult.find_last_of("\\");
	stringstream ss1, ss2;
	ss1 << T.inNum;
	string inputNum = ss1.str();
	ss2 << T.outNum;
	string outputNum = ss2.str();
	
	//make dot file-------------------------------------------------------------------------------
	string verilogFile(finalResult);
	char verilogBuffer[300];
	size_t verilogEnd, verilogSize, verilogFound;
	verilogFound = verilogFile.find_last_of("\\");
	//basicFile.insert(basicFile.size(), "allnodes.txt");
	//---------------------------------------------------------------------------------------------
	int inVerilog = 0;
	int tempVerilog = 0;
	int verilogNode;
	bool InorOut = false;
	char integerBuffer[10];
	conflictNode* lastGeneration = new conflictNode();
	for(int finalGen = 0; finalGen < DFGSize; finalGen++)
	{
		if(SortedNodes[finalGen]._Op.compare("IN") && SortedNodes[finalGen]._Op.compare("CONS")) //nodes that are not in and cons
			lastGeneration->Nodes.push_back(finalGen); //stores all input and constant nodes

	}
	
	vector<int> verilogInputs;
	vector<int> verilogOutputs;
	//string for file operation
	ofstream fileLastDOT;
	string strLastDOT(verilogFile);
	strLastDOT.erase(verilogFound + 1, strLastDOT.size() - verilogFound - 1);
	strLastDOT.insert(strLastDOT.size(), "finalDOT.dot");
	verilogSize = strLastDOT.size();
	verilogEnd = strLastDOT.copy(verilogBuffer, verilogSize, 0);
	verilogBuffer[verilogEnd] = '\0';
	//end of file operation
	fileLastDOT.open(verilogBuffer);
	fileLastDOT << "digraph G {\n";

	int outputNodes = 1000;

	findVerilogInputs(lastGeneration, verilogInputs, SortedNodes);
	findVerilogOutputs(lastGeneration, verilogOutputs, SortedNodes);


	for(int inNodes = 0; inNodes < verilogInputs.size(); inNodes++)
	{
		if(!SortedNodes[verilogInputs[inNodes]]._Op.compare("CONS"))
			fileLastDOT << SortedNodes[verilogInputs[inNodes]].lastID << " [label=\"" << "CONS" << "\"];\n";
		else
			fileLastDOT << SortedNodes[verilogInputs[inNodes]].lastID << " [label=\"" << "IN" << "\"];\n";
	}


	for(int outNodes = 0; outNodes < verilogOutputs.size(); outNodes++)
	{
		fileLastDOT << outputNodes << " [label=\"" << "OUT" << "\"];\n";
		outputNodes++;
	}

	for(verilogNode = 0; verilogNode < lastGeneration->Nodes.size(); verilogNode++)
	{
		fileLastDOT << SortedNodes[lastGeneration->Nodes[verilogNode]].lastID << " [label=\"" << SortedNodes[lastGeneration->Nodes[verilogNode]]._Op << "\"];\n";

	}

	for(int traceInput = 0; traceInput < verilogInputs.size(); traceInput++)
		printDotty(lastGeneration, verilogInputs[traceInput], fileLastDOT, SortedNodes);
	
	for(int outNodes = 1000; outNodes < outputNodes; outNodes++)
	{
		fileLastDOT << SortedNodes[verilogOutputs[outNodes - 1000]].lastID << " -> " << outNodes << ";\n";
	}
	fileLastDOT << "}";
	fileLastDOT.close();

	string commandLine("dot -Tgif ");
	commandLine.insert(commandLine.size(), strLastDOT);
	commandLine.insert(commandLine.size(), " -o ");
	strLastDOT.insert(strLastDOT.size(), ".gif");
	commandLine.insert(commandLine.size(), strLastDOT);
	const char * finalCommand = commandLine.c_str();
	//system(finalCommand);
	
	for(int inDFG = 0; inDFG < DFGSize; inDFG++)
	{
		SortedNodes[inDFG].isTraced = false;
	}




	verilogInputs.clear();
	verilogOutputs.clear();
	delete lastGeneration;

//generate dotty file for CIs
	while(inVerilog < conflictNumber)
	{   
		bool dontADD = false;
		if(conflictArray[inVerilog].finalInclude)
		{
			ofstream verilogCI;
			string verilogTemp(verilogFile);
			verilogTemp.erase(verilogFound + 1, verilogTemp.size() - verilogFound - 1);
			verilogTemp.insert(verilogTemp.size(), itoa(verilogGenerated, integerBuffer, 10));
			verilogGenerated++;
			verilogTemp.insert(verilogTemp.size(), "-Verilog-");
			verilogTemp.insert(verilogTemp.size(), inputNum);
			verilogTemp.insert(verilogTemp.size(), "-");
			verilogTemp.insert(verilogTemp.size(), outputNum);
			verilogTemp.insert(verilogTemp.size(), ".dot");
			verilogSize = verilogTemp.size();
			verilogEnd = verilogTemp.copy(verilogBuffer, verilogSize, 0);
			verilogBuffer[verilogEnd] = '\0';
			verilogCI.open(verilogBuffer);
			verilogCI << "digraph G {\n";


			
			findVerilogInputs(&conflictArray[inVerilog], verilogInputs, SortedNodes);
			findVerilogOutputs(&conflictArray[inVerilog], verilogOutputs, SortedNodes);
			//generate nodes------------------------------------------------------------------------
			tempVerilog = conflictArray[inVerilog].TemNum;
			bool exist = false;
			bool noOut = false;
			outputNodes = 1000;
			for(int inNodes = 0; inNodes < verilogInputs.size(); inNodes++)
			{
				if(!SortedNodes[verilogInputs[inNodes]]._Op.compare("CONS"))
					verilogCI << SortedNodes[verilogInputs[inNodes]].lastID << " [label=\"" << "CONS" << "\"];\n";
				else
					verilogCI << SortedNodes[verilogInputs[inNodes]].lastID << " [label=\"" << "IN" << "\"];\n";
			}

			for(int outNodes = 0; outNodes < verilogOutputs.size(); outNodes++)
			{
				verilogCI << outputNodes << " [label=\"" << "OUT" << "\"];\n";
				outputNodes++;
			}
			for(verilogNode = 0; verilogNode < conflictArray[inVerilog].Nodes.size(); verilogNode++)
			{
				verilogCI << SortedNodes[conflictArray[inVerilog].Nodes[verilogNode]].lastID << " [label=\"" << SortedNodes[conflictArray[inVerilog].Nodes[verilogNode]]._Op << "\"];\n";

			}
			for(int traceInput = 0; traceInput < verilogInputs.size(); traceInput++)
				printDotty(&conflictArray[inVerilog], verilogInputs[traceInput], verilogCI, SortedNodes);
			
			for(int outNodes = 1000; outNodes < outputNodes; outNodes++)
			{
				verilogCI << SortedNodes[verilogOutputs[outNodes - 1000]].lastID << " -> " << outNodes << ";\n";
			}
			//end generate nodes-------------------------------------------------------------------------
			while(tempVerilog == conflictArray[inVerilog].TemNum)
			{
				dontADD = true;
				inVerilog++;	
			}
			verilogCI << "}";
			verilogInputs.clear();
			verilogOutputs.clear();
			verilogCI.close();
			string commandLine("dot -Tgif ");
			commandLine.insert(commandLine.size(), verilogTemp);
			commandLine.insert(commandLine.size(), " -o ");
			verilogTemp.insert(verilogTemp.size(), ".gif");
			commandLine.insert(commandLine.size(), verilogTemp);
			const char * finalCommand = commandLine.c_str();
			//system(finalCommand);
			
			for(int inDFG = 0; inDFG < DFGSize; inDFG++)
			{
				SortedNodes[inDFG].isTraced = false;
			}
		}
		if(!dontADD)
			inVerilog++;
	}
    //---------------------------------------------------------------------------
	//combine all results
	ofstream fileCmbResults;
	double sumArea = 0.0;
	string strCmbResults;
	int sum21, sum31, sum32, sum42, sum44, sum63, sum84, sumAll = 0;
	//string strCmbResults(finalResult);
	size_t sizeCmbFound, sizeCmbEnd;
	string strFuncName, strAlgName;
	char bufCmbResult[200];
	char tempInt[10];
	if(firstIteration)
	{
		strCmbResults.insert(strCmbResults.size(), finalResult);
		sizeCmbFound =  strCmbResults.find_last_of("\\");
		strCmbResults.erase(sizeCmbFound , strCmbResults.size() - sizeCmbFound);
			
		sizeCmbFound =  strCmbResults.find_last_of("\\");
		strCmbResults.erase(sizeCmbFound, strCmbResults.size() - sizeCmbFound);

		sizeCmbFound =  strCmbResults.find_last_of("\\");
		strFuncName.append(strCmbResults, sizeCmbFound + 1, strCmbResults.size());
		genFuncName.insert(genFuncName.size(), strFuncName);
		strCmbResults.erase(sizeCmbFound, strCmbResults.size() - sizeCmbFound);

		sizeCmbFound =  strCmbResults.find_last_of("\\");
		strCmbResults.erase(sizeCmbFound, strCmbResults.size() - sizeCmbFound);
		
		sizeCmbFound =  strCmbResults.find_last_of("\\");
		strAlgName.append(strCmbResults, sizeCmbFound + 1, strCmbResults.size());
		genAlgName.insert(genAlgName.size(), strAlgName);

		strCmbResults.insert(strCmbResults.size(), "\\");
		if(localAlgorithm == true)
			strCmbResults.insert(strCmbResults.size(), "local");
		else
			strCmbResults.insert(strCmbResults.size(), "global");
		strCmbResults.insert(strCmbResults.size(), "-");
		strCmbResults.insert(strCmbResults.size(), itoa(T.inNum, tempInt, 10));
		strCmbResults.insert(strCmbResults.size(), itoa(T.outNum, tempInt, 10));
		strCmbResults.insert(strCmbResults.size(), "-");
		strCmbResults.insert(strCmbResults.size(), "inR");
		strCmbResults.insert(strCmbResults.size(), itoa(inRegisterConstraint, tempInt, 10));
		strCmbResults.insert(strCmbResults.size(), "-");
		strCmbResults.insert(strCmbResults.size(), "outR");
		strCmbResults.insert(strCmbResults.size(), itoa(outRegisterConstraint, tempInt, 10));
		strCmbResults.insert(strCmbResults.size(), "-");
		strCmbResults.insert(strCmbResults.size(), "objfunc");
		strCmbResults.insert(strCmbResults.size(), itoa(objectiveFunction, tempInt, 10));

		genCombineResults.insert(genCombineResults.size(), strCmbResults);
		mkdir(strCmbResults.c_str());
	}
	else
		strCmbResults.insert(strCmbResults.size(), genCombineResults);
	strCmbResults.insert(strCmbResults.size(), "\\allResults");
	strCmbResults.insert(strCmbResults.size(), inputNum);
	strCmbResults.insert(strCmbResults.size(), "-");
	strCmbResults.insert(strCmbResults.size(), outputNum);
	strCmbResults.insert(strCmbResults.size(), ".txt");

	sizeCmbEnd = strCmbResults.copy(bufCmbResult, strCmbResults.size(), 0);
	bufCmbResult[sizeCmbEnd] = '\0';

		
	fileCmbResults.open(bufCmbResult, ios::app);

	fstream fileCmbResultEmpty;
	fileCmbResultEmpty.open(bufCmbResult, ios::in);
	string bufTempCombine;

	//--------------------------------------------------------------------

	//--------------------------------------------------------------------

	//fileCmbResults.seekp(0, ios::end);
	char operationChar[100];
	double cmbArea, cmbHWLatency, cmbSumArea; 
	int	   cmbNode, cmbIteration, cmbOccurance, cmbInput, cmbOutput, cmb21, cmb31, cmb32, cmb42;
	int     cmbSum21, cmbSum31, cmbSum32, cmbSum42;
	int fileLine = 0;
	char tempLine[20];

	while(!fileCmbResultEmpty.eof())
	{
		getline(fileCmbResultEmpty, bufTempCombine);
		fileLine++;
	}
	if(fileLine == 1 && bufTempCombine.size() == 0)
	{//empty file
		fileCmbResults << "#" << genAlgName << '\n';
		fileCmbResults << "#" <<"\tOperation\tArea\tHW-Latency\tNodes\tIteration\toccurrance\tInput\tOutputs\t2_1\t3_1\t3_2\t4_2\t4_4\t6_3\t8_4\t" << "RegCons(" << inRegisterConstraint << "-" << outRegisterConstraint << ")\tSum(Area)\tSum(2_1)\tSum(3_1)\tSum(3_2)\tSum(4_2)\tSum(4_4)\tSum(6_3)\tSum(8_4)\t" << "Sum(RegCons" << inRegisterConstraint << "-" << outRegisterConstraint << ")\n";
		sumArea = 0.0;
		sum21 = 0;
		sum31 = 0;
		sum32 = 0;
		sum42 = 0;
		sum44 = 0;
		sum63 = 0;
		sum84 = 0;
		sumAll = 0;
	}
	else
	{
		FILE* fileCombine;
		fileCombine = fopen(bufCmbResult, "r");
		char tempCombine[1000];
		char* tempString;
		size_t terminatorFound;
		sumArea = 0.0;
		sum21 = 0;
		sum31 = 0;
		sum32 = 0;
		sum42 = 0;
		sum44 = 0;
		sum63 = 0;
		sum84 = 0;
		sumAll = 0;
		
		//while(fscanf(fileCombine, "%s %20LF %20LF %d %d %d %d %d %d %d %d %20LF %d %d %d %d", operationChar, &cmbArea,
		//	&cmbHWLatency, &cmbNode, &cmbIteration, &cmbInput, &cmbOutput, &cmb21, &cmb31, &cmb32, &cmb42,
		//	&cmbSumArea, &cmbSum21, &cmbSum31, &cmbSum32, &cmbSum42))
		while(!feof(fileCombine))
		{
			fgets(tempCombine, 1000, fileCombine);
			tempString = strtok(tempCombine, "\t");
			string checkTerminator(tempCombine);

			int i = 0;
			while(tempString != NULL)
			{
				terminatorFound = checkTerminator.find("#");
				if(terminatorFound != string::npos)
					break;
				if(tempString == "#")
					break;
				tempString = strtok(NULL, "\t");
				if(tempString != NULL)
				{
					if(i == 13)
						sumArea = atof(tempString);
					if(i == 14)
						sum21 = atoi(tempString);
					if(i == 15)
						sum31 = atoi(tempString);
					if(i == 16)
						sum32  = atoi(tempString);
					if(i == 17)
						sum42 = atoi(tempString);
					if(i == 18)
						sum44 = atoi(tempString);
					if(i == 19)
						sum63 = atoi(tempString);
					if(i == 20)
						sum84 = atoi(tempString);
					if(i == 21)
						sumAll = atoi(tempString);
				}
				i++;
			}
		}

		fclose(fileCombine);

	}
	fileCmbResults << "#" <<genFuncName << '\n';
	//---------------------------------------------------------------------------
	finalResult.erase(finalFound + 1, finalResult.size() - finalFound - 1);
	finalResult.insert(finalResult.size(), "finalResult");
	finalResult.insert(finalResult.size(), inputNum);
	finalResult.insert(finalResult.size(), "-");
	finalResult.insert(finalResult.size(), outputNum);
	finalResult.insert(finalResult.size(), ".txt");
	finalfilelen = finalResult.size();
	finalakhar = finalResult.copy(buffer,finalfilelen,0);
	buffer[finalakhar] = '\0';
	fileCI.open(buffer);
	fileCI.precision(15);
	fileCI << "#Input Constraint: " << T.inNum << "\n";
	fileCI << "#Output Constraint: " << T.outNum << "\n";
	fileCI << "#Level: " << levelOfTree + 1 << "\n";
	fileCI << "#Selection Time(microSecond): " << selectTimer << '\n';
	fileCI << "#Operation\tArea\tHW-Latency\tNodes\tIteration\tOccurance\tInput\tOutputs\t2_1\t3_1\t3_2\t4_2\t4_4\t6_3\t8_4\t"<< "RegCons(" << inRegisterConstraint << "-" << outRegisterConstraint << ")\tMerit\n";

	int newTemplate = 0;
	int previousTemplate = 0;
	int TemplateOccurance[1000];
	memset(TemplateOccurance, 0, 1000*sizeof(int));
	double TemplateIteration[1000];
	memset(TemplateIteration, 0.0, 1000*sizeof(double));
	int TempPlace = 0;
	bool avalinConflict = false;
	bool nextTemplate = false;
	for(int intNum = 0; intNum < conflictNumber; intNum++)
	{
		if(conflictArray[intNum].finalInclude)
		{
			if(avalinConflict == false)
			{
				previousTemplate = conflictArray[intNum].TemNum;
				avalinConflict = true;
			}
			if(conflictArray[intNum].TemNum != previousTemplate)
				newTemplate++;
			TemplateOccurance[newTemplate]++;
			TemplateIteration[newTemplate] += ceil(conflictArray[intNum].Iteration/17179.0);
			previousTemplate = conflictArray[intNum].TemNum;
		}
	}

	bool flagOutLoop = false;
	double finalArea;
	newTemplate = 0;
	previousTemplate = 0;
	int int21, int31, int32, int42, int44, int63, int84, intAll = 0;
	int inConstraint21, outConstraint21 = 0;
	int inConstraint31, outConstraint31 = 0;
	int inConstraint32, outConstraint32 = 0;
	int inConstraint42, outConstraint42 = 0;
	int inConstraint44, outConstraint44 = 0;
	int inConstraint63, outConstraint63 = 0;
	int inConstraint84, outConstraint84 = 0;
	int inConstraintAll, outConstraintAll = 0;
	int total21, total31, total32, total42 = 0;
	double totalMerit;
	for(int inTemp = 0; inTemp < conflictNumber; inTemp++)
		if(conflictArray[inTemp].finalInclude)
		{

			//previousTemplate = conflictArray[inTemp].TemNum;
			if(previousTemplate != conflictArray[inTemp].TemNum && flagOutLoop == true)
			{
				//total21 += int21;
				//total31 += int31;
				//total32 += int32;
				//total42 += int42;
         
				//cout << total21 << '\t';
				//cout << total31<< '\t';
				//cout << total32<< '\t';
				//cout << total42<< '\t';

				//cout << '\n';


				/*if(int21 < 0)
				{
					fileCI << 0;
					fileCmbResults << 0;
					int21 = 0;
				}
				else
				{
					fileCI << int21;
					fileCmbResults << int21;
				}
				fileCI << '\t';
				fileCmbResults << '\t';

				if(int31 < 0)
				{
					fileCI << 0;
					fileCmbResults << 0;
					int31 = 0;
				}
				else
				{
					fileCI << int31;
					fileCmbResults << int31;
				}
				fileCI << '\t';
				fileCmbResults << '\t';

				if(int32 < 0)
				{
					fileCI << 0;
					fileCmbResults << 0;
					int32 = 0;
				}
				else
				{
					fileCI << int32;
					fileCmbResults << int32;
				}
				fileCI << '\t';
				fileCmbResults << '\t';

				if(int42 < 0)
				{
					fileCI << 0;
					fileCmbResults << 0;
					int42 = 0;
				}
				else
				{
					fileCmbResults << int42;
					fileCI << int42;
				}
				fileCI << '\t';
				fileCmbResults << '\t';


			    if(int44 < 0)
				{
					fileCI << 0;
					fileCmbResults << 0;
					int44 = 0;
				}
				else
				{
					fileCmbResults << int44;
					fileCI << int44;
				}
				fileCI << '\t';
				fileCmbResults << '\t';

				if(int63 < 0)
				{
					fileCI << 0;
					fileCmbResults << 0;
					int63 = 0;
				}
				else
				{
					fileCmbResults << int63;
					fileCI << int63;
				}
				fileCI << '\t';
				fileCmbResults << '\t';

				if(int84 < 0)
				{
					fileCI << 0;
					fileCmbResults << 0;
					int84 = 0;
				}
				else
				{
					fileCmbResults << int84;
					fileCI << int84;
				}
				fileCI << '\t';
				fileCmbResults << '\t';

				if(intAll < 0)
				{
					fileCI << 0;
					fileCmbResults << 0;
					intAll = 0;
				}
				else
				{
					fileCmbResults << intAll;
					fileCI << intAll;
				}
				fileCI << '\t'; -->> empty */
				fileCmbResults << '\t';

				fileCmbResults << finalArea + sumArea;
				fileCmbResults << '\t';

				fileCmbResults << int21 + sum21;
				fileCmbResults << '\t';

				fileCmbResults << int31 + sum31;
				fileCmbResults << '\t';

				fileCmbResults << int32 + sum32;
				fileCmbResults << '\t';

				fileCmbResults << int42 + sum42;
				fileCmbResults << '\t';

				fileCmbResults << int44 + sum44;
				fileCmbResults << '\t';

				fileCmbResults << int63 + sum63;
				fileCmbResults << '\t';

				fileCmbResults << int84 + sum84;
				fileCmbResults << '\t';

				fileCmbResults << intAll + sumAll;
				fileCmbResults << '\t';

				fileCmbResults << '\n';

				sumArea = finalArea + sumArea;
				sum21 = int21 + sum21;
				sum31 = int31 + sum31;
				sum32 = int32 + sum32;
				sum42 = int42 + sum42;
				sum44 = int44 + sum44;
				sum63 = int63 + sum63;
				sum84 = int84 + sum84;
				sumAll = intAll + sumAll;
				fileCI << totalMerit;
				fileCI << '\t';
				fileCI << '\n';

				flagOutLoop = false;
				//fileCI << "*************************************************************************************\n";
			}
			if(previousTemplate == conflictArray[inTemp].TemNum && flagOutLoop == true)
			{
				//inConstraint21 = conflictArray[inTemp].inputNum > 2 ? ceil(((conflictArray[inTemp].inputNum) - 2)/2) : 0;
				//outConstraint21 = conflictArray[inTemp].outputNum > 1 ? ceil(((conflictArray[inTemp].outputNum) - 1)/1) : 0;
				
				//inConstraint31 = conflictArray[inTemp].inputNum > 3 ? ceil(((conflictArray[inTemp].inputNum) - 3)/3) : 0;
				//outConstraint31 = conflictArray[inTemp].outputNum > 1 ? ceil(((conflictArray[inTemp].outputNum) - 1)/1) : 0;

				//inConstraint32 = conflictArray[inTemp].inputNum > 3 ? ceil(((conflictArray[inTemp].inputNum) - 3)/3) : 0;
				//outConstraint32 = conflictArray[inTemp].outputNum > 2 ? ceil(((conflictArray[inTemp].outputNum) - 2)/2) : 0;

				//inConstraint42 = conflictArray[inTemp].inputNum > 4 ? ceil(((conflictArray[inTemp].inputNum) - 4)/4) : 0;
				//outConstraint42 = conflictArray[inTemp].outputNum > 2 ? ceil(((conflictArray[inTemp].outputNum) - 2)/2) : 0;
				totalMerit += conflictArray[inTemp].Merit;
				int21 += (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint21 - outConstraint21);
				int31 += (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint31 - outConstraint31);
				int32 += (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint32 - outConstraint32);
				int42 += (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint42 - outConstraint42);
				int44 += (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint44 - outConstraint44);
				int63 += (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint63 - outConstraint63);
				int84 += (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint84 - outConstraint84);
				intAll += (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraintAll - outConstraintAll);

			}
			if(flagOutLoop == false)
			{
				previousTemplate = conflictArray[inTemp].TemNum;
				int21 = 0;
				int31 = 0;
				int32 = 0;
				int42 = 0;
				int44 = 0;
				int63 = 0;
				int84 = 0;
				intAll = 0;
				totalMerit = 0;

				//fileCI << "*************************************************************************************\n";
				//fileCI << TemplateOccurance[0] << '\n';

				//fileCI << "Template: " << newTemplate << '\n';
				finalArea = 0;
				fileCmbResults << '\t';
				for(int opI = 0; opI < conflictArray[inTemp].Nodes.size(); opI++)
				{
					finalArea += SortedNodes[conflictArray[inTemp].Nodes[opI]]._Area;
					
					fileCI <<"@" << conflictArray[inTemp].Nodes[opI] << "(" << SortedNodes[conflictArray[inTemp].Nodes[opI]]._Op;
					fileCmbResults << conflictArray[inTemp].Nodes[opI] << "(" << SortedNodes[conflictArray[inTemp].Nodes[opI]]._Op;
					if(SortedNodes[conflictArray[inTemp].Nodes[opI]].hasCons)
					{
						fileCI << "_C";
						fileCmbResults << "_C";
					}
					fileCI << ")" << ", ";
					fileCmbResults << ")" << ", ";
				}
				
				/*fileCI << "\t";-->>*/fileCI << "\n";
				fileCmbResults << "\t";
				fileCI << conflictArray[inTemp].Merit;
                fileCI << "\t";
				
				fileCI << conflictArray[inTemp].Iteration;
				fileCI << "\t";
				
				fileCI << finalArea;
				fileCI << "\t";

				fileCmbResults << finalArea;
				fileCmbResults << "\t";
				
				fileCI << conflictArray[inTemp].HWLatency;
				fileCI << "\t";

				fileCmbResults << conflictArray[inTemp].HWLatency;
				fileCmbResults << "\t";
				
				fileCI << conflictArray[inTemp].Nodes.size();
				fileCI << "\t";

				fileCmbResults << conflictArray[inTemp].Nodes.size();
				fileCmbResults << "\t";
				
				fileCI << TemplateIteration[newTemplate];//ceil(conflictArray[inTemp].Iteration / 17179.0);
				fileCI << "\t";

				fileCmbResults << TemplateIteration[newTemplate];//ceil(conflictArray[inTemp].Iteration / 17179.0);
				fileCmbResults << "\t";

				/*fileCI << TemplateOccurance[newTemplate]; -- >> empty fileCI << "\t";*/
				fileCmbResults << TemplateOccurance[newTemplate];
				newTemplate++;
				
				fileCmbResults << "\t";

				
				totalMerit = conflictArray[inTemp].Merit;
				/*fileCI << conflictArray[inTemp].inputNum;-->>*///CIarray[ciarrayI]->inputNum;
				//fileCI << "\t";

				fileCmbResults << conflictArray[inTemp].inputNum;//CIarray[ciarrayI]->inputNum;
				fileCmbResults << "\t";

				/*fileCI << conflictArray[inTemp].outputNum;
				fileCI << "\t"; -->>*/
				
				fileCmbResults << conflictArray[inTemp].outputNum;
				fileCmbResults << "\t";

				
				inConstraint21 = conflictArray[inTemp].inputNum > 2 ? ceil(((conflictArray[inTemp].inputNum) - 2)/2.0) : 0;
				outConstraint21 = conflictArray[inTemp].outputNum > 1 ? ceil(((conflictArray[inTemp].outputNum) - 1)/1.0) : 0;
				
				inConstraint31 = conflictArray[inTemp].inputNum > 3 ? ceil(((conflictArray[inTemp].inputNum) - 3)/3.0) : 0;
				outConstraint31 = conflictArray[inTemp].outputNum > 1 ? ceil(((conflictArray[inTemp].outputNum) - 1)/1.0) : 0;

				inConstraint32 = conflictArray[inTemp].inputNum > 3 ? ceil(((conflictArray[inTemp].inputNum) - 3)/3.0) : 0;
				outConstraint32 = conflictArray[inTemp].outputNum > 2 ? ceil(((conflictArray[inTemp].outputNum) - 2)/2.0) : 0;

				inConstraint42 = conflictArray[inTemp].inputNum > 4 ? ceil(((conflictArray[inTemp].inputNum) - 4)/4.0) : 0;
				outConstraint42 = conflictArray[inTemp].outputNum > 2 ? ceil(((conflictArray[inTemp].outputNum) - 2)/2.0) : 0;

				inConstraint44 = conflictArray[inTemp].inputNum > 4 ? ceil(((conflictArray[inTemp].inputNum) - 4)/4.0) : 0;
				outConstraint44 = conflictArray[inTemp].outputNum > 4 ? ceil(((conflictArray[inTemp].outputNum) - 4)/4.0) : 0;

				inConstraint63 = conflictArray[inTemp].inputNum > 6 ? ceil(((conflictArray[inTemp].inputNum) - 6)/6.0) : 0;
				outConstraint63 = conflictArray[inTemp].outputNum > 3 ? ceil(((conflictArray[inTemp].outputNum) - 3)/3.0) : 0;

				inConstraint84 = conflictArray[inTemp].inputNum > 8 ? ceil(((conflictArray[inTemp].inputNum) - 8)/8.0) : 0;
				outConstraint84 = conflictArray[inTemp].outputNum > 4 ? ceil(((conflictArray[inTemp].outputNum) - 4)/4.0) : 0;

				inConstraintAll = conflictArray[inTemp].inputNum > inRegisterConstraint ? ceil(((conflictArray[inTemp].inputNum) - inRegisterConstraint)/(double)inRegisterConstraint) : 0;
				outConstraintAll = conflictArray[inTemp].outputNum > outRegisterConstraint ? ceil(((conflictArray[inTemp].outputNum) - outRegisterConstraint)/(double)outRegisterConstraint) : 0;

				int21 = (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint21 - outConstraint21);
				int31 = (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint31 - outConstraint31);
				int32 = (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint32 - outConstraint32);
				int42 = (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint42 - outConstraint42);
				int44 = (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint44 - outConstraint44);
				int63 = (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint63 - outConstraint63);
				int84 = (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraint84 - outConstraint84);
				intAll = (int)ceil((conflictArray[inTemp].Iteration / 17179.0)) * (conflictArray[inTemp].Nodes.size() - 1 - inConstraintAll - outConstraintAll);
				
				flagOutLoop = true;
			}
		}


		//total21 += int21;
	    //total31 += int31;
		//total32 += int32;
		//total42 += int42;

		/*if(int21 < 0)
		{
			fileCI << 0;
			fileCmbResults << 0;
			int21 = 0;
		}			
		else
		{
			fileCI << int21;
			fileCmbResults << int21;
		}

		fileCI << '\t';
		fileCmbResults << '\t';

		if(int31 < 0)
		{
			fileCI << 0;
			fileCmbResults << 0;
			int31 = 0;
		}
		else
		{
			fileCmbResults << int31;
			fileCI << int31;
		}
		fileCI << '\t';
		fileCmbResults << '\t';

		if(int32 < 0)
		{
			fileCmbResults << 0;
			fileCI << 0;
			int32 = 0;
		}
		else
		{
			fileCmbResults << int32;
			fileCI << int32;
		}
		fileCI << '\t';

		
		fileCmbResults << '\t';

		if(int42 < 0)
		{
			fileCI << 0;
			fileCmbResults << 0;
			int42 = 0;
		}
		else
		{
			fileCmbResults << int42;
			fileCI << int42;
		}
		fileCI << '\t';
		fileCmbResults << '\t';

		if(int44 < 0)
		{
			fileCI << 0;
			fileCmbResults << 0;
			int44 = 0;
		}
		else
		{
			fileCmbResults << int44;
			fileCI << int44;
		}
		fileCI << '\t';
		fileCmbResults << '\t';

		if(int63 < 0)
		{
			fileCI << 0;
			fileCmbResults << 0;
			int63 = 0;
		}
		else
		{
			fileCmbResults << int63;
			fileCI << int63;
		}
		fileCI << '\t';
		fileCmbResults << '\t';

		if(int84 < 0)
		{
			fileCI << 0;
			fileCmbResults << 0;
			int84 = 0;
		}
		else
		{
			fileCmbResults << int84;
			fileCI << int84;
		}
		fileCI << '\t';
		fileCmbResults << '\t';

		if(intAll < 0)
		{
			fileCI << 0;
			fileCmbResults << 0;
			intAll = 0;
		}
		else
		{
			fileCmbResults << intAll;
			fileCI << intAll;
		}
		fileCI << '\t';
		fileCmbResults << '\t';
		-->>empty */
		fileCmbResults << finalArea + sumArea;
		fileCmbResults << '\t';

		fileCmbResults << int21 + sum21;
		fileCmbResults << '\t';

		fileCmbResults << int31 + sum31;
		fileCmbResults << '\t';

		fileCmbResults << int32 + sum32;
		fileCmbResults << '\t';

		fileCmbResults << int42 + sum42;
		fileCmbResults << '\t';

		fileCmbResults << int44 + sum44;
		fileCmbResults << '\t';

		fileCmbResults << int63 + sum63;
		fileCmbResults << '\t';

		fileCmbResults << int84 + sum84;
		fileCmbResults << '\t';

		fileCmbResults << intAll + sumAll;
		fileCmbResults << '\t';
		fileCmbResults << '\n';

		fileCI << totalMerit;
		fileCI << '\t';
		fileCI << '\n';

		//fileCI << total21 << '\t' << total31 << '\t'<< total32 << '\t'<< total42;

		
		fileCI.close();
		fileCmbResults.close();

		//cin >> totalMerit;


        cout << "print result finished>>>>>>>>>\n";
		//CIarray.clear();
}
//------------------------------------------------------------------------------------------------------------------------------
int findMISMerit(bool* MIScand, conflictNode* conflictArray, int Size, double& Area, 
				 int inRegisterConstraint, int outRegisterConstraint, int clockConstraint)
{//begin of findMISMerit
	//1)MIScand:			a boolean array that shows inclusion state of a conflict node in MIS
	//2)conflictArray:		an array that has all conflict nodes
	//3)Size:				size of conflict array
	//4)Area:				area(used as output)
	//-------------------------------------------------------------------------------------------------

	//variable definition-------
	int i;
	int Merit = 0;
	Area = 0;
	vector<int> TemplateNumber;
	int inConstraint;
	int outConstraint;
	bool find = false;
	//--------------------------

	//loop over all conflict nodes-------------------------------------------------------------------------------------
	for(i = 0; i < Size; i++)
	{
		if(MIScand[i] == true)//if this node is in MIS
		{
			inConstraint = conflictArray[i].inputNum > inRegisterConstraint ? ceil(((conflictArray[i].inputNum) - inRegisterConstraint)/(double)inRegisterConstraint) : 0;
			outConstraint = conflictArray[i].outputNum > outRegisterConstraint ? ceil(((conflictArray[i].outputNum) - outRegisterConstraint)/(double)outRegisterConstraint) : 0;
			
			Merit += conflictArray[i].Iteration * (conflictArray[i].Nodes.size() - clockConstraint - inConstraint - outConstraint);//sum merit of all conflict nodes that are in MIS
			TemplateNumber.push_back(conflictArray[i].TemNum);//push template number of this conflict node to an array
			
			//check if this template number was in TemplateNumber array or not
			for(int j = 0; j < TemplateNumber.size() -1 ; j++)
			{
				if(conflictArray[i].TemNum == TemplateNumber[j])
					find = true;
			}
			if(find == false)//if it is a new template add area to that
				Area  += conflictArray[i].Nodes.size();//conflictArray[i].Area;
 		}
		find = false;
	}
	//------------------------------------------------------------------------------------------------------------------
	TemplateNumber.clear();//clear the template number

	return Merit;

}
//------------------------------------------------------------------------------------------------------------------------------
int findWorstMISMerit(bool* MIScand, conflictNode* conflictArray, int Size)
{
	//it is not used in our algorithm
	int i;
	double Area;
	vector<int> TemplateNumber;
	//int nowMerit = findMISMerit(MIScand, conflictArray, Size, Area);
	int nowMerit;
	bool find = false;
	for(i = Size - 1; i >= 0; i--)
	{
		if(MIScand[i] == false)
		{
			nowMerit += (int)conflictArray[i].Merit;
			TemplateNumber.push_back(conflictArray[i].TemNum);
			for(int j = 0; j < TemplateNumber.size() - 1; j++)
			{
				if(conflictArray[i].TemNum == TemplateNumber[j])
					find = true;
			}
			if(find == false)
				Area  += conflictArray[i].Nodes.size();//conflictArray[i].Area;
		}
		else
			break;

		find = false;
	}
	return (nowMerit/Area);
}
//------------------------------------------------------------------------------------------------------------------------------
bool checkNeighbor(bool* MIScand, conflictNode* conflictArray, int Size)
{//begin of checkNeighbor
	//1)MIScand:			a boolean array that shows inclusion state of a conflict node in MIS
	//2)conflictArray:		an array that has all conflict nodes
	//3)Size:				size of conflict array
	//-------------------------------------------------------------------------------------------------
	
	//variable definition--
	int i, j, k;
	bool checkSim = false;
	bool Result = true;
	//---------------------


	for(i = 0; i < Size; i++)
	{
		if(MIScand[i] == true)
		{
			//loop over outSimilarities of this node
			for(j = 0; j < conflictArray[i].outSimilarities.size(); j++)
			{
				for(k = 0; k < Size; k++)
				{
					if(MIScand[conflictArray[i].outSimilarities[j]])//check if it is in MIScand
					{
						checkSim = true;
						Result = false;
						break;
					}
				}
				if(checkSim == true)
				{
					Result = false;
					break;
				}
			}

			if(checkSim == true)
			{
				Result = false;
				break;
			}

			//loop over outSimilarities of this node
			for(j = 0; j < conflictArray[i].inSimilarities.size(); j++)
			{
				for(k = 0; k < Size; k++)
				{
					if(MIScand[conflictArray[i].inSimilarities[j]])//check if it is in MIScand
					{
						checkSim = true;
						Result = false;
						break;
					}
				}
				if(checkSim == true)
				{
					Result = false;
					break;
				}
			}
			
		}
		if(checkSim == true)
		{
			Result = false;
			break;
		}
	}

	return Result;
}
//-----------------------------------------------------------------------------------------
void maxMIS0(conflictNode* conflictArray, int Size, int inRegisterConstraint, int outRegisterConstraint, int clockConstraint)
{//begin of maxMIS(exact MWIS function that search over all conflict nodes with some prune)
	//1)conflictArray:		an array that has all conflict nodes
	//2)Size:				size of conflict array
	//-------------------------------------------------------------------------------------------------
	

	//a boolean array that shows the state of inclusion of conflictnodes
	bool* MIScand = new bool[Size];
	memset(MIScand, 0, Size * sizeof(bool));
	//------------------------------------------------------------------

	//a boolean array that shows the last node in binary search tree
	bool* LastMIS = new bool[Size];
	memset(LastMIS, 0, Size * sizeof(bool));
	LastMIS[Size - 1] = true;
	//--------------------------------------------------------------
	
	//variable definition------------------------
	MIScand[0] = true;
	int CurrentLevel = 0;
	int numberOfMIS = 0;
	int i;
	double bestMerit = -1000000000.0;
	bool* bestMIScand = new bool[Size];
	memset(bestMIScand, 0, Size * sizeof(bool));
	double tmpMerit = 0.0;
	//-------------------------------------------

	//conditions for traverse binary tree
	bool DescendFlag = true;
	bool RightFlag = true;
	bool ContinueFlag = true;
	bool EqualFlag;
	bool Conditions;
	int MaxCount = 0;
	int lenMax = 0;
	double Area;
	int uniqueMerit;
	//-----------------------------------

	//to count the timing of exact MWIS function
	precision_timer generationTime;
	generationTime.start();
	//------------------------------------------
	while(ContinueFlag)
	{//check continue Flag
		if(DescendFlag)
		{//check descend flag
			Conditions = true;
			if (Conditions == true)
			{//prune condition
				//call checkNeighbor function that return false if a neighbor of this node are in MWIS
				Conditions = checkNeighbor(MIScand, conflictArray, Size);
			}
			if(Conditions)
			{
				Area = 0.0;
				uniqueMerit = findMISMerit(MIScand, conflictArray, Size, Area, inRegisterConstraint, outRegisterConstraint, clockConstraint);
				if((uniqueMerit/Area) > bestMerit)
				{
					bestMerit = (uniqueMerit/Area);
					for(int j = 0; j < Size; j++)
					{//store the best MWIS in a boolean array
						if(MIScand[j] == true)
							bestMIScand[j] = true;
						else
							bestMIScand[j] = false;
					}

				}

				if(CurrentLevel == Size - 1)
				{//traverse tree ascend
					DescendFlag = false;
					MIScand[CurrentLevel] = false;
					CurrentLevel--;
				}
				else
				{
					CurrentLevel++;
					MIScand[CurrentLevel] = true;
				}
			}
			else
			{
				DescendFlag = false;
				MIScand[CurrentLevel] = false;
				CurrentLevel--;
			}
		}
		else
		{
			if(RightFlag)
				if(CurrentLevel != Size - 2)
				{
					CurrentLevel = CurrentLevel + 2;
					MIScand[CurrentLevel] = true;
					DescendFlag = true;
				}
				else
				{
					if(MIScand[CurrentLevel])
						MIScand[CurrentLevel] = false;
					else
						RightFlag = false;
					CurrentLevel--;
				}
			else
			{
				if(MIScand[CurrentLevel])
				{
					MIScand[CurrentLevel] = false;
					RightFlag = true;
				}
				CurrentLevel--;
			}
		}

		EqualFlag = true;
		for(int i = 0; i < Size; i++)
		{
			if(MIScand[i] != LastMIS[i])
			{
				EqualFlag = false;
				break;
			}
		}
		if(EqualFlag)
			ContinueFlag = false;
	}

	Conditions = true;
	if (Conditions == true)
	{//prune condition
		Conditions = checkNeighbor(MIScand, conflictArray, Size);
	}
	if(Conditions)
	{
		double Area;
		int uniqueMerit = findMISMerit(MIScand, conflictArray, Size, Area, inRegisterConstraint, outRegisterConstraint, clockConstraint);
		if( (uniqueMerit/Area) > bestMerit)
		{
			bestMerit = (uniqueMerit/Area);
			for(int j = 0; j < Size; j++)
			{//store the best MWIS in a boolean array
				if(MIScand[j] == true)
					bestMIScand[j] = true;
				else
					bestMIScand[j] = false;
			}
		}
	}

	for(int i = 0; i < Size; i++)
	{
		if(bestMIScand[i])
			conflictArray[i].finalInclude = true;

	}
}
//---------------------------------------------------------------------------------------------------------------------------
void maxMIS1(conflictNode* conflictArray, int Size, int inRegisterConstraint, int outRegisterConstraint, int clockConstraint)
{//begin of maxMIS(exact MWIS function that search over all conflict nodes with some prune)
	//1)conflictArray:		an array that has all conflict nodes
	//2)Size:				size of conflict array
	//-------------------------------------------------------------------------------------------------
	

	//a boolean array that shows the state of inclusion of conflictnodes
	bool* MIScand = new bool[Size];
	memset(MIScand, 0, Size * sizeof(bool));
	//------------------------------------------------------------------

	//a boolean array that shows the last node in binary search tree
	bool* LastMIS = new bool[Size];
	memset(LastMIS, 0, Size * sizeof(bool));
	LastMIS[Size - 1] = true;
	//--------------------------------------------------------------
	
	//variable definition------------------------
	MIScand[0] = true;
	int CurrentLevel = 0;
	int numberOfMIS = 0;
	int i;
	double bestMerit = -1000000000.0;
	bool* bestMIScand = new bool[Size];
	memset(bestMIScand, 0, Size * sizeof(bool));
	double tmpMerit = 0.0;
	//-------------------------------------------

	//conditions for traverse binary tree
	bool DescendFlag = true;
	bool RightFlag = true;
	bool ContinueFlag = true;
	bool EqualFlag;
	bool Conditions;
	int MaxCount = 0;
	int lenMax = 0;
	double Area;
	int uniqueMerit;
	//-----------------------------------

	//to count the timing of exact MWIS function
	precision_timer generationTime;
	generationTime.start();
	//------------------------------------------
	while(ContinueFlag)
	{//check continue Flag
		if(DescendFlag)
		{//check descend flag
			Conditions = true;
			if (Conditions == true)
			{//prune condition
				//call checkNeighbor function that return false if a neighbor of this node are in MWIS
				Conditions = checkNeighbor(MIScand, conflictArray, Size);
			}
			if(Conditions)
			{
				Area = 0.0;
				uniqueMerit = findMISMerit(MIScand, conflictArray, Size, Area, inRegisterConstraint, outRegisterConstraint, clockConstraint);
				if((uniqueMerit) > bestMerit)
				{
					bestMerit = (uniqueMerit);
					for(int j = 0; j < Size; j++)
					{//store the best MWIS in a boolean array
						if(MIScand[j] == true)
							bestMIScand[j] = true;
						else
							bestMIScand[j] = false;
					}

				}

				if(CurrentLevel == Size - 1)
				{//traverse tree ascend
					DescendFlag = false;
					MIScand[CurrentLevel] = false;
					CurrentLevel--;
				}
				else
				{
					CurrentLevel++;
					MIScand[CurrentLevel] = true;
				}
			}
			else
			{
				DescendFlag = false;
				MIScand[CurrentLevel] = false;
				CurrentLevel--;
			}
		}
		else
		{
			if(RightFlag)
				if(CurrentLevel != Size - 2)
				{
					CurrentLevel = CurrentLevel + 2;
					MIScand[CurrentLevel] = true;
					DescendFlag = true;
				}
				else
				{
					if(MIScand[CurrentLevel])
						MIScand[CurrentLevel] = false;
					else
						RightFlag = false;
					CurrentLevel--;
				}
			else
			{
				if(MIScand[CurrentLevel])
				{
					MIScand[CurrentLevel] = false;
					RightFlag = true;
				}
				CurrentLevel--;
			}
		}

		EqualFlag = true;
		for(int i = 0; i < Size; i++)
		{
			if(MIScand[i] != LastMIS[i])
			{
				EqualFlag = false;
				break;
			}
		}
		if(EqualFlag)
			ContinueFlag = false;
	}

	Conditions = true;
	if (Conditions == true)
	{//prune condition
		Conditions = checkNeighbor(MIScand, conflictArray, Size);
	}
	if(Conditions)
	{
		double Area;
		int uniqueMerit = findMISMerit(MIScand, conflictArray, Size, Area, inRegisterConstraint, outRegisterConstraint, clockConstraint);
		if( (uniqueMerit) > bestMerit)
		{
			bestMerit = (uniqueMerit);
			for(int j = 0; j < Size; j++)
			{//store the best MWIS in a boolean array
				if(MIScand[j] == true)
					bestMIScand[j] = true;
				else
					bestMIScand[j] = false;
			}
		}
	}

	bool templateNumber[10000];
	memset(templateNumber, false, 10000 * sizeof(bool));
	for(int i = 0; i < Size; i++)
	{
		if(bestMIScand[i] && !templateNumber[conflictArray[i].TemNum])
		{
			conflictArray[i].finalInclude = true;
			templateNumber[conflictArray[i].TemNum] = true; 
		}

	}
}
//---------------------------------------------------------------------------------------------------------------
void objectiveFunction9(conflictNode* conflictArray, int allRepitation, vector<int> MaxIS[], int NumTemplate, int clockConstraint, 
						string basicFile, int RFin, int RFout)
{
	//performance with RF constraint
	int objNum;
	int withinTmp;

	int inConstraint; 
	int outConstraint;
	double objective = -1000000.0;
	int delTemp = 1000000000;
	bool flagIter = false;
	int performanceArea = 0;
	int tmpObj, i;
	//unsigned test : 4;
	cout <<"in object function 9(Performance/Power)\n";
	

	for(objNum = 0; objNum < NumTemplate; objNum++)
	{//first for
		objective = -1000000.0;
		delTemp = 1000000000;
		flagIter = false;
		cout << "NumTemplate: " << objNum << "-" << NumTemplate << '\n';
		for(withinTmp = 0; withinTmp < NumTemplate; withinTmp++)
		{
			//cout << "withinTmp: " << withinTmp << '\n';
			tmpObj = 0;
			if(MaxIS[withinTmp].size() != 0)
			{
				inConstraint = conflictArray[MaxIS[withinTmp][0]].inputNum > RFin ? ceil(((conflictArray[MaxIS[withinTmp][0]].inputNum) - RFin)/(double)RFin) : 0;
				outConstraint = conflictArray[MaxIS[withinTmp][0]].outputNum > RFout ? ceil(((conflictArray[MaxIS[withinTmp][0]].outputNum) - RFout)/(double)RFout) : 0;
				
				for(i = 0; i < MaxIS[withinTmp].size(); i++)
				{
					tmpObj += (conflictArray[MaxIS[withinTmp][i]].Iteration * (conflictArray[MaxIS[withinTmp][0]].Nodes.size() - clockConstraint - inConstraint - outConstraint)) / ( 1 + conflictArray[MaxIS[withinTmp][0]].Power) ;
				}

				if(tmpObj > objective && tmpObj > 0)
				{
					flagIter = true;
					objective = tmpObj;
					delTemp = withinTmp;
				}
				else if(tmpObj <= 0)
				{
					MaxIS[withinTmp].clear();
				}
			}
		}
		if(flagIter)
		{
			unsigned int inTmp = 0;
			bool* simillarity = new bool[allRepitation];
			memset(simillarity, FALSE, sizeof(bool) * allRepitation);
			for(int xi = 0; xi < allRepitation; xi++)
				if(findSimilarities(conflictArray[MaxIS[delTemp][inTmp]].Nodes, conflictArray[xi].Nodes) && MaxIS[delTemp][inTmp] != xi)
					simillarity[xi] = true;

			while(!MaxIS[delTemp].empty())
			{
				for(unsigned int simi = 0; simi < conflictArray[MaxIS[delTemp][inTmp]].outTemp; simi++)
				{
					for(int inTemp = 0; inTemp < NumTemplate; inTemp++)
					{
						int inMax = 0;
						bool what = false;
						while(inMax != MaxIS[inTemp].size())
						{
							what = false;
							if(simillarity[MaxIS[inTemp][inMax]])
							{
								MaxIS[inTemp].erase(MaxIS[inTemp].begin()+inMax);
								inMax = 0;
								what = true;
								if(MaxIS[inTemp].size() == 0)
									break;
							}
							if(what == false)
								inMax++;
						}
					}
				}
				conflictArray[MaxIS[delTemp][inTmp]].finalInclude = true;
				MaxIS[delTemp].erase(MaxIS[delTemp].begin()+inTmp);
			}

			delete[] simillarity;
			simillarity = NULL;
		}
	}
	cout <<"end object function 4\n";
}
//---------------------------------------------------------------------------------------------------------------
void objectiveFunction8(conflictNode* conflictArray,vector<int> MaxIS[], int NumTemplate, int clockConstraint, string basicFile)
{
	int objNum;
	int withinTmp;

	int		performanceArea = 0;
	double	objective = -1000000.0;
	int		delTemp = 1000000000;
	bool	flagIter = false;
	double	tmpObj = 0.0;
	int i;
	cout << "begining of the objectionFunc8(Power)\n";
	for(objNum = 0; objNum < NumTemplate; objNum++)
	{//first for
		objective = -1000000.0;
		delTemp = 1000000000;
		flagIter = false;
		for(withinTmp = 0; withinTmp < NumTemplate; withinTmp++)
		{
			if(MaxIS[withinTmp].size() != 0)
			{
				tmpObj = 0.0;
				for(i = 0; i < MaxIS[withinTmp].size(); i++)
				{
					tmpObj +=  1/(1+conflictArray[MaxIS[withinTmp][i]].Power);
				}
				if(tmpObj > objective)
				{
					flagIter = true;
					objective = tmpObj;
					delTemp = withinTmp;
				}
			}
		}
		cout << "middle of the objectionFunc8\n";
		if(flagIter)
		{
			unsigned int inTmp = 0;
			while(!MaxIS[delTemp].empty())
			{
				for(unsigned int simi = 0; simi < conflictArray[MaxIS[delTemp][inTmp]].outSimilarities.size(); simi++)
				{
					for(int inTemp = 0; inTemp < NumTemplate; inTemp++)
					{
						int inMax = 0;
						bool what = false;
						while(inMax != MaxIS[inTemp].size())
						{
							what = false;
							if(conflictArray[MaxIS[delTemp][inTmp]].outSimilarities[simi] == MaxIS[inTemp][inMax])
							{
								MaxIS[inTemp].erase(MaxIS[inTemp].begin()+inMax);
								inMax = 0;
								what = true;
								if(MaxIS[inTemp].size() == 0)
									break;
							}
							if(what == false)
								inMax++;
						}
					}
				}
				conflictArray[MaxIS[delTemp][inTmp]].finalInclude = true;
				MaxIS[delTemp].erase(MaxIS[delTemp].begin()+inTmp);
			}
		}
	}
	cout << "end of the objectionFunc3\n";
}
//------------------------------------------------------------------------------------------------------------------------------
void objectiveFunction7(conflictNode* conflictArray,vector<int> MaxIS[], int NumTemplate, int clockConstraint,string basicFile, int RFin, int RFout)
{
	//performance with RF constraint
	int objNum;
	int withinTmp;

	int inConstraint, i, j; //= conflictArray[inTemp].inputNum > RFin ? ceil(((conflictArray[inTemp].inputNum) - RFin)/(double)RFin) : 0;
	int outConstraint; //= conflictArray[inTemp].outputNum > RFout ? ceil(((conflictArray[inTemp].outputNum) - RFout)/(double)RFout) : 0;

	int performanceArea = 0;
	double tmpObj = 0.0;

	double allPerformance = 0.0;
	double allArea = 0.0;
	double tmpPerformance = 0.0;
	double tmpArea = 0.0;
	double inPerformance = 0.0;
	double outPerformance = 0.0;
	double inArea = 0.0;
	double outArea = 0.0;

	for(objNum = 0; objNum < NumTemplate; objNum++)
	{//first for
		double objective = -1000000.0;
		int delTemp = 1000000000;
		bool flagIter = false;
		for(withinTmp = 0; withinTmp < NumTemplate; withinTmp++)
		{
			tmpObj = 0.0;
			bool tempArea[10000];
			memset(tempArea, false, 10000 * sizeof(bool));

			if(MaxIS[withinTmp].size() != 0)
			{
				inConstraint = conflictArray[MaxIS[withinTmp][0]].inputNum > RFin ? ceil(((conflictArray[MaxIS[withinTmp][0]].inputNum) - RFin)/(double)RFin) : 0;
				outConstraint = conflictArray[MaxIS[withinTmp][0]].outputNum > RFout ? ceil(((conflictArray[MaxIS[withinTmp][0]].outputNum) - RFout)/(double)RFout) : 0;
				outPerformance = 0.0;
				outArea = 0.0;

				for(i = 0; i < MaxIS[withinTmp].size(); i++)
				{
					tmpObj += conflictArray[MaxIS[withinTmp][i]].Iteration * (conflictArray[MaxIS[withinTmp][0]].Nodes.size() - clockConstraint - inConstraint - outConstraint); 
					for(j = 0; j < conflictArray[MaxIS[withinTmp][i]].outSimilarities.size(); j++)
						if(conflictArray[conflictArray[MaxIS[withinTmp][i]].outSimilarities[j]].MISinclude)
						{
							outPerformance += conflictArray[conflictArray[MaxIS[withinTmp][i]].outSimilarities[j]].Iteration * (conflictArray[conflictArray[MaxIS[withinTmp][i]].outSimilarities[j]].Nodes.size() 
								- clockConstraint - inConstraint - outConstraint);
							if(tempArea[conflictArray[conflictArray[MaxIS[withinTmp][i]].outSimilarities[j]].TemNum] == false)
							{
								outArea += conflictArray[conflictArray[MaxIS[withinTmp][i]].outSimilarities[j]].Area;
								tempArea[conflictArray[conflictArray[MaxIS[withinTmp][i]].outSimilarities[j]].TemNum] = true;
							}
						}
				}

				
				

				if((tmpObj + allPerformance - outPerformance) / (conflictArray[MaxIS[withinTmp][0]].Area + allArea + outArea) > objective && tmpObj > 0)
				{
				//if(tmpObj > objective && tmpObj > 0)
					flagIter = true;
					objective = (tmpObj + allPerformance - outPerformance) / (conflictArray[MaxIS[withinTmp][0]].Area + allArea + outArea);
					tmpPerformance = tmpObj + allPerformance - outPerformance;
					tmpArea = conflictArray[MaxIS[withinTmp][0]].Area + allArea + outArea;
					delTemp = withinTmp;
				}
				else if(tmpObj <= 0)
				{
					MaxIS[withinTmp].clear();
				}
			}
		}
		if(flagIter)
		{
			unsigned int inTmp = 0;
			allPerformance = tmpPerformance;
			allArea	= tmpArea;
			while(!MaxIS[delTemp].empty())
			{
				for(unsigned int simi = 0; simi < conflictArray[MaxIS[delTemp][inTmp]].outSimilarities.size(); simi++)
				{
					for(int inTemp = 0; inTemp < NumTemplate; inTemp++)
					{
						int inMax = 0;
						bool what = false;
						while(inMax != MaxIS[inTemp].size())
						{
							what = false;
							if(conflictArray[MaxIS[delTemp][inTmp]].outSimilarities[simi] == MaxIS[inTemp][inMax])
							{
								MaxIS[inTemp].erase(MaxIS[inTemp].begin()+inMax);
								conflictArray[conflictArray[MaxIS[delTemp][inTmp]].outSimilarities[simi]].MISinclude = false;
								inMax = 0;
								what = true;
								if(MaxIS[inTemp].size() == 0)
									break;
							}
							if(what == false)
								inMax++;
						}
					}
				}
				conflictArray[MaxIS[delTemp][inTmp]].finalInclude = true;
				conflictArray[MaxIS[delTemp][inTmp]].MISinclude = false;
				MaxIS[delTemp].erase(MaxIS[delTemp].begin()+inTmp);
			}
		}
	}
}
//------------------------------------------------------------------------------------------------------------------------------
void objectiveFunction6(conflictNode* conflictArray,vector<int> MaxIS[], int NumTemplate, int clockConstraint,string basicFile, int RFin, int RFout)
{
	//performance with RF constraint
	int objNum;
	int withinTmp;

	int inConstraint, i; //= conflictArray[inTemp].inputNum > RFin ? ceil(((conflictArray[inTemp].inputNum) - RFin)/(double)RFin) : 0;
	int outConstraint; //= conflictArray[inTemp].outputNum > RFout ? ceil(((conflictArray[inTemp].outputNum) - RFout)/(double)RFout) : 0;

	int performanceArea = 0;
	double tmpObj = 0.0;

	double allPerformance = 0.0;
	double allArea = 0.0;
	double tmpPerformance = 0.0;
	double tmpArea = 0.0;

	for(objNum = 0; objNum < NumTemplate; objNum++)
	{//first for
		double objective = -1000000.0;
		int delTemp = 1000000000;
		bool flagIter = false;
		for(withinTmp = 0; withinTmp < NumTemplate; withinTmp++)
		{
			tmpObj = 0.0;
			if(MaxIS[withinTmp].size() != 0)
			{
				inConstraint = conflictArray[MaxIS[withinTmp][0]].inputNum > RFin ? ceil(((conflictArray[MaxIS[withinTmp][0]].inputNum) - RFin)/(double)RFin) : 0;
				outConstraint = conflictArray[MaxIS[withinTmp][0]].outputNum > RFout ? ceil(((conflictArray[MaxIS[withinTmp][0]].outputNum) - RFout)/(double)RFout) : 0;
				
				for(i = 0; i < MaxIS[withinTmp].size(); i++)
				{
					tmpObj += conflictArray[MaxIS[withinTmp][i]].Iteration * (conflictArray[MaxIS[withinTmp][0]].Nodes.size() - clockConstraint - inConstraint - outConstraint); 
				}
				//tmpObj = tmpObj / conflictArray[MaxIS[withinTmp][0]].Area;

				if((tmpObj + allPerformance) / (conflictArray[MaxIS[withinTmp][0]].Area + allArea) > objective && tmpObj > 0)
				{
				//if(tmpObj > objective && tmpObj > 0)
					flagIter = true;
					objective = (tmpObj + allPerformance) / (conflictArray[MaxIS[withinTmp][0]].Area + allArea);
					tmpPerformance = tmpObj + allPerformance;
					tmpArea = conflictArray[MaxIS[withinTmp][0]].Area + allArea;
					delTemp = withinTmp;
				}
				else if(tmpObj <= 0)
				{
					MaxIS[withinTmp].clear();
				}
			}
		}
		if(flagIter)
		{
			unsigned int inTmp = 0;
			allPerformance = tmpPerformance;
			allArea	= tmpArea;
			while(!MaxIS[delTemp].empty())
			{
				for(unsigned int simi = 0; simi < conflictArray[MaxIS[delTemp][inTmp]].outSimilarities.size(); simi++)
				{
					for(int inTemp = 0; inTemp < NumTemplate; inTemp++)
					{
						int inMax = 0;
						bool what = false;
						while(inMax != MaxIS[inTemp].size())
						{
							what = false;
							if(conflictArray[MaxIS[delTemp][inTmp]].outSimilarities[simi] == MaxIS[inTemp][inMax])
							{
								MaxIS[inTemp].erase(MaxIS[inTemp].begin()+inMax);
								inMax = 0;
								what = true;
								if(MaxIS[inTemp].size() == 0)
									break;
							}
							if(what == false)
								inMax++;
						}
					}
				}
				conflictArray[MaxIS[delTemp][inTmp]].finalInclude = true;
				MaxIS[delTemp].erase(MaxIS[delTemp].begin()+inTmp);
			}
		}
	}
}
//------------------------------------------------------------------------------------------------------------------------------
void objectiveFunction5(conflictNode* conflictArray,vector<int> MaxIS[], int NumTemplate, int clockConstraint,string basicFile, int RFin, int RFout)
{
	//performance with RF constraint
	int objNum;
	int withinTmp;

	int inConstraint, i; //= conflictArray[inTemp].inputNum > RFin ? ceil(((conflictArray[inTemp].inputNum) - RFin)/(double)RFin) : 0;
	int outConstraint; //= conflictArray[inTemp].outputNum > RFout ? ceil(((conflictArray[inTemp].outputNum) - RFout)/(double)RFout) : 0;

	int performanceArea = 0;
	double tmpObj = 0.0;

	for(objNum = 0; objNum < NumTemplate; objNum++)
	{//first for
		double objective = -1000000.0;
		int delTemp = 1000000000;
		bool flagIter = false;
		for(withinTmp = 0; withinTmp < NumTemplate; withinTmp++)
		{
			tmpObj = 0.0;
			if(MaxIS[withinTmp].size() != 0)
			{
				inConstraint = conflictArray[MaxIS[withinTmp][0]].inputNum > RFin ? ceil(((conflictArray[MaxIS[withinTmp][0]].inputNum) - RFin)/(double)RFin) : 0;
				outConstraint = conflictArray[MaxIS[withinTmp][0]].outputNum > RFout ? ceil(((conflictArray[MaxIS[withinTmp][0]].outputNum) - RFout)/(double)RFout) : 0;
				
				for(i = 0; i < MaxIS[withinTmp].size(); i++)
				{
					tmpObj += conflictArray[MaxIS[withinTmp][i]].Iteration * (conflictArray[MaxIS[withinTmp][0]].Nodes.size() - clockConstraint - inConstraint - outConstraint); 
				}
				tmpObj = tmpObj / conflictArray[MaxIS[withinTmp][0]].Area;

				if(tmpObj > objective && tmpObj > 0)
				{
					flagIter = true;
					objective = tmpObj;
					delTemp = withinTmp;
				}
				else if(tmpObj <= 0)
				{
					MaxIS[withinTmp].clear();
				}
			}
		}
		if(flagIter)
		{
			unsigned int inTmp = 0;
			while(!MaxIS[delTemp].empty())
			{
				for(unsigned int simi = 0; simi < conflictArray[MaxIS[delTemp][inTmp]].outSimilarities.size(); simi++)
				{
					for(int inTemp = 0; inTemp < NumTemplate; inTemp++)
					{
						int inMax = 0;
						bool what = false;
						while(inMax != MaxIS[inTemp].size())
						{
							what = false;
							if(conflictArray[MaxIS[delTemp][inTmp]].outSimilarities[simi] == MaxIS[inTemp][inMax])
							{
								MaxIS[inTemp].erase(MaxIS[inTemp].begin()+inMax);
								inMax = 0;
								what = true;
								if(MaxIS[inTemp].size() == 0)
									break;
							}
							if(what == false)
								inMax++;
						}
					}
				}
				conflictArray[MaxIS[delTemp][inTmp]].finalInclude = true;
				MaxIS[delTemp].erase(MaxIS[delTemp].begin()+inTmp);
			}
		}
	}
}
//---------------------------------------------------------------------------------------------------------------
void objectiveFunction41(conflictNode* conflictArray, int allRepitation, vector<int> MaxIS[], int NumTemplate, int clockConstraint, 
						string basicFile, int RFin, int RFout)
{
	//performance with RF constraint
	//WMIN
	int objNum;
	int withinTmp;

	int inConstraint; 
	int outConstraint;
	double objective = -1000000.0;
	int delTemp = 1000000000;
	bool flagIter = false;
	int performanceArea = 0;
	int tmpObj, i;
	//unsigned test : 4;
	cout <<"in object function 4\n";
	

	for(objNum = 0; objNum < NumTemplate; objNum++)
	{//first for
		objective = -1000000.0;
		delTemp = 1000000000;
		flagIter = false;
		int outEdges = 0;
		int tmpOutEdges = 0;
		cout << "NumTemplate: " << objNum << "-" << NumTemplate << '\n';
		for(withinTmp = 0; withinTmp < NumTemplate; withinTmp++)
		{
			//cout << "withinTmp: " << withinTmp << '\n';
			tmpObj = 0;
			outEdges = 0;
			if(MaxIS[withinTmp].size() != 0)
			{
				inConstraint = conflictArray[MaxIS[withinTmp][0]].inputNum > RFin ? ceil(((conflictArray[MaxIS[withinTmp][0]].inputNum) - RFin)/(double)RFin) : 0;
				outConstraint = conflictArray[MaxIS[withinTmp][0]].outputNum > RFout ? ceil(((conflictArray[MaxIS[withinTmp][0]].outputNum) - RFout)/(double)RFout) : 0;
				
				for(i = 0; i < MaxIS[withinTmp].size(); i++)
				{
					tmpObj += conflictArray[MaxIS[withinTmp][i]].Iteration * (conflictArray[MaxIS[withinTmp][0]].Nodes.size() - clockConstraint - inConstraint - outConstraint); 
					//outEdges += conflictArray[MaxIS[withinTmp][i]].outTemp;
				}

				if(tmpObj > objective && tmpObj > 0)
				{
					flagIter = true;
					objective = tmpObj;
					delTemp = withinTmp;
				}
				else if(tmpObj <= 0)
				{
					MaxIS[withinTmp].clear();
				}
			}
		}
		if(flagIter)
		{
			unsigned int inTmp = 0;
			bool* simillarity = new bool[allRepitation]; //check similarity with other conflict nodes
			while(!MaxIS[delTemp].empty())
			{
				memset(simillarity, FALSE, sizeof(bool) * allRepitation);
				for(int xi = 0; xi < allRepitation; xi++)
					if(findSimilarities(conflictArray[MaxIS[delTemp][inTmp]].Nodes, conflictArray[xi].Nodes) && MaxIS[delTemp][inTmp] != xi)
						simillarity[xi] = true;
				for(unsigned int simi = 0; simi < conflictArray[MaxIS[delTemp][inTmp]].outTemp; simi++) //changed in 4/10
				{
					cout << "number of outTemp:>>>>>>>>>>>> " << conflictArray[MaxIS[delTemp][inTmp]].outTemp << '\n';
					for(int inTemp = 0; inTemp < NumTemplate; inTemp++)
					{
						int inMax = 0;
						bool what = false;
						while(inMax != MaxIS[inTemp].size())
						{
							what = false;
							if(simillarity[MaxIS[inTemp][inMax]])
							{
								MaxIS[inTemp].erase(MaxIS[inTemp].begin()+inMax);
								inMax = 0;
								what = true;
								if(MaxIS[inTemp].size() == 0)
									break;
							}
							if(what == false)
								inMax++;
						}
					}
				}
				conflictArray[MaxIS[delTemp][inTmp]].finalInclude = true;
				MaxIS[delTemp].erase(MaxIS[delTemp].begin()+inTmp);
			}

			delete[] simillarity;
			simillarity = NULL;
		}
	}
	cout <<"end object function 4\n";
}
//--------------------------------------------------------------------------------------------------------------------------------------------
void objectiveFunction42(conflictNode* conflictArray, int allRepitation, vector<int> MaxIS[], int NumTemplate, int clockConstraint, 
						string basicFile, int RFin, int RFout)
{
	//wmax
	//performance with RF constraint
	int objNum;
	int withinTmp;

	int inConstraint; 
	int outConstraint;
	double objective = 1000000.0;
	int delTemp = 1000000000;
	bool flagIter = false;
	int performanceArea = 0;
	int tmpObj, i;
	//unsigned test : 4;
	cout <<"in object function 4\n";
	

	for(objNum = 0; objNum < NumTemplate; objNum++)
	{//first for
		objective = 1.79769e+308;
		delTemp = 1000000000;
		flagIter = false;
		int outEdges = 0;
		int tmpOutEdges = 0;
		cout << "NumTemplate: " << objNum << "-" << NumTemplate << '\n';
		for(withinTmp = 0; withinTmp < NumTemplate; withinTmp++)
		{
			//cout << "withinTmp: " << withinTmp << '\n';
			tmpObj = 0;
			outEdges = 0;
			if(MaxIS[withinTmp].size() != 0)
			{
				inConstraint = conflictArray[MaxIS[withinTmp][0]].inputNum > RFin ? ceil(((conflictArray[MaxIS[withinTmp][0]].inputNum) - RFin)/(double)RFin) : 0;
				outConstraint = conflictArray[MaxIS[withinTmp][0]].outputNum > RFout ? ceil(((conflictArray[MaxIS[withinTmp][0]].outputNum) - RFout)/(double)RFout) : 0;
				
				for(i = 0; i < MaxIS[withinTmp].size(); i++)
				{
					tmpObj += conflictArray[MaxIS[withinTmp][i]].Iteration * (conflictArray[MaxIS[withinTmp][0]].Nodes.size() - clockConstraint - inConstraint - outConstraint); 
					//outEdges += conflictArray[MaxIS[withinTmp][i]].outTemp;
				}

				if(tmpObj  < objective && tmpObj > 0)
				{
					flagIter = true;
					objective = tmpObj;
					delTemp = withinTmp;
				}
				else if(tmpObj <= 0)
				{
					MaxIS[withinTmp].clear();
				}
			}
		}
		if(flagIter)
		{
			unsigned int inTmp = 0;
			bool* simillarity = new bool[allRepitation]; //check similarity with other conflict nodes
			while(!MaxIS[delTemp].empty())
			{
				memset(simillarity, FALSE, sizeof(bool) * allRepitation);
				for(int xi = 0; xi < allRepitation; xi++)
					if(findSimilarities(conflictArray[MaxIS[delTemp][inTmp]].Nodes, conflictArray[xi].Nodes) && MaxIS[delTemp][inTmp] != xi)
						simillarity[xi] = true;
				for(unsigned int simi = 0; simi < conflictArray[MaxIS[delTemp][inTmp]].outTemp; simi++) //changed in 4/10
				{
					cout << "number of outTemp:>>>>>>>>>>>> " << conflictArray[MaxIS[delTemp][inTmp]].outTemp << '\n';
					for(int inTemp = 0; inTemp < NumTemplate; inTemp++)
					{
						int inMax = 0;
						bool what = false;
						while(inMax != MaxIS[inTemp].size())
						{
							what = false;
							if(simillarity[MaxIS[inTemp][inMax]])
							{
								MaxIS[inTemp].erase(MaxIS[inTemp].begin()+inMax);
								inMax = 0;
								what = true;
								if(MaxIS[inTemp].size() == 0)
									break;
							}
							if(what == false)
								inMax++;
						}
					}
				}
				conflictArray[MaxIS[delTemp][inTmp]].finalInclude = true;
				MaxIS[delTemp].erase(MaxIS[delTemp].begin()+inTmp);
			}

			delete[] simillarity;
			simillarity = NULL;
		}
	}
	cout <<"end object function 4\n";
}
//---------------------------------------------------------------------------------------------------------------
void objectiveFunction43(conflictNode* conflictArray, int allRepitation, vector<int> MaxIS[], int NumTemplate, int clockConstraint, 
						string basicFile, int RFin, int RFout)
{
	//GWMIN
	//performance with RF constraint
	int objNum;
	int withinTmp;

	int inConstraint; 
	int outConstraint;
	double objective = -1000000.0;
	int delTemp = 1000000000;
	bool flagIter = false;
	int performanceArea = 0;
	int tmpObj, i;
	//unsigned test : 4;
	cout <<"in object function 4\n";
	

	for(objNum = 0; objNum < NumTemplate; objNum++)
	{//first for
		objective = -1000000.0;
		delTemp = 1000000000;
		flagIter = false;
		int outEdges = 0;
		int tmpOutEdges = 0;
		cout << "NumTemplate: " << objNum << "-" << NumTemplate << '\n';
		for(withinTmp = 0; withinTmp < NumTemplate; withinTmp++)
		{
			//cout << "withinTmp: " << withinTmp << '\n';
			tmpObj = 0;
			outEdges = 0;
			if(MaxIS[withinTmp].size() != 0)
			{
				inConstraint = conflictArray[MaxIS[withinTmp][0]].inputNum > RFin ? ceil(((conflictArray[MaxIS[withinTmp][0]].inputNum) - RFin)/(double)RFin) : 0;
				outConstraint = conflictArray[MaxIS[withinTmp][0]].outputNum > RFout ? ceil(((conflictArray[MaxIS[withinTmp][0]].outputNum) - RFout)/(double)RFout) : 0;
				
				for(i = 0; i < MaxIS[withinTmp].size(); i++)
				{
					tmpObj += conflictArray[MaxIS[withinTmp][i]].Iteration * (conflictArray[MaxIS[withinTmp][0]].Nodes.size() - clockConstraint - inConstraint - outConstraint); 
					outEdges += conflictArray[MaxIS[withinTmp][i]].outTemp;
				}

				if((tmpObj / (double) (outEdges + 1)) > objective && tmpObj > 0)
				{
					flagIter = true;
					objective = (tmpObj / (double) (outEdges+1));
					delTemp = withinTmp;
				}
				else if(tmpObj <= 0)
				{
					MaxIS[withinTmp].clear();
				}
			}
		}
		if(flagIter)
		{
			unsigned int inTmp = 0;
			bool* simillarity = new bool[allRepitation]; //check similarity with other conflict nodes
			while(!MaxIS[delTemp].empty())
			{
				memset(simillarity, FALSE, sizeof(bool) * allRepitation);
				for(int xi = 0; xi < allRepitation; xi++)
					if(findSimilarities(conflictArray[MaxIS[delTemp][inTmp]].Nodes, conflictArray[xi].Nodes) && MaxIS[delTemp][inTmp] != xi)
						simillarity[xi] = true;
				for(unsigned int simi = 0; simi < conflictArray[MaxIS[delTemp][inTmp]].outTemp; simi++) //changed in 4/10
				{
					cout << "number of outTemp:>>>>>>>>>>>> " << conflictArray[MaxIS[delTemp][inTmp]].outTemp << '\n';
					for(int inTemp = 0; inTemp < NumTemplate; inTemp++)
					{
						int inMax = 0;
						bool what = false;
						while(inMax != MaxIS[inTemp].size())
						{
							what = false;
							if(simillarity[MaxIS[inTemp][inMax]])
							{
								for(int i = 0; i < conflictArray[MaxIS[inTemp][inMax]].outTemp; i++)
									conflictArray[conflictArray[MaxIS[inTemp][inMax]].outSimilarities[i]].outTemp--;
								MaxIS[inTemp].erase(MaxIS[inTemp].begin()+inMax);
								inMax = 0;
								what = true;
								if(MaxIS[inTemp].size() == 0)
									break;
							}
							if(what == false)
								inMax++;
						}
					}
				}
				conflictArray[MaxIS[delTemp][inTmp]].finalInclude = true;
				MaxIS[delTemp].erase(MaxIS[delTemp].begin()+inTmp);
			}

			delete[] simillarity;
			simillarity = NULL;
		}
	}
	cout <<"end object function 4\n";
}
//---------------------------------------------------------------------------------------------------------------
void objectiveFunction44(conflictNode* conflictArray, int allRepitation, vector<int> MaxIS[], int NumTemplate, int clockConstraint, 
						string basicFile, int RFin, int RFout)
{
	//GWMAX
	//performance with RF constraint
	int objNum;
	int withinTmp;

	int inConstraint; 
	int outConstraint;
	double objective = 10000000.0;
	int delTemp = 1000000000;
	bool flagIter = false;
	int performanceArea = 0;
	int tmpObj, i;
	//unsigned test : 4;
	cout <<"in object function 4\n";
	

	for(objNum = 0; objNum < NumTemplate; objNum++)
	{//first for
		objective = 1.79769e+308;
		delTemp = 1000000000;
		flagIter = false;
		int outEdges = 0;
		int tmpOutEdges = 0;
		cout << "NumTemplate: " << objNum << "-" << NumTemplate << '\n';
		for(withinTmp = 0; withinTmp < NumTemplate; withinTmp++)
		{
			//cout << "withinTmp: " << withinTmp << '\n';
			tmpObj = 0;
			outEdges = 0;
			if(MaxIS[withinTmp].size() != 0)
			{
				inConstraint = conflictArray[MaxIS[withinTmp][0]].inputNum > RFin ? ceil(((conflictArray[MaxIS[withinTmp][0]].inputNum) - RFin)/(double)RFin) : 0;
				outConstraint = conflictArray[MaxIS[withinTmp][0]].outputNum > RFout ? ceil(((conflictArray[MaxIS[withinTmp][0]].outputNum) - RFout)/(double)RFout) : 0;
				
				for(i = 0; i < MaxIS[withinTmp].size(); i++)
				{
					tmpObj += conflictArray[MaxIS[withinTmp][i]].Iteration * (conflictArray[MaxIS[withinTmp][0]].Nodes.size() - clockConstraint - inConstraint - outConstraint); 
					outEdges += conflictArray[MaxIS[withinTmp][i]].outTemp;
				}

				if((tmpObj / (double) (outEdges + 1)) < objective && tmpObj > 0)
				{
					flagIter = true;
					objective = (tmpObj / (double) (outEdges+1));
					delTemp = withinTmp;
				}
				else if(tmpObj <= 0)
				{
					MaxIS[withinTmp].clear();
				}
			}
		}
		if(flagIter)
		{
			unsigned int inTmp = 0;
			bool* simillarity = new bool[allRepitation]; //check similarity with other conflict nodes
			while(!MaxIS[delTemp].empty())
			{
				memset(simillarity, FALSE, sizeof(bool) * allRepitation);
				for(int xi = 0; xi < allRepitation; xi++)
					if(findSimilarities(conflictArray[MaxIS[delTemp][inTmp]].Nodes, conflictArray[xi].Nodes) && MaxIS[delTemp][inTmp] != xi)
						simillarity[xi] = true;
				for(unsigned int simi = 0; simi < conflictArray[MaxIS[delTemp][inTmp]].outTemp; simi++) //changed in 4/10
				{
					cout << "number of outTemp:>>>>>>>>>>>> " << conflictArray[MaxIS[delTemp][inTmp]].outTemp << '\n';
					for(int inTemp = 0; inTemp < NumTemplate; inTemp++)
					{
						int inMax = 0;
						bool what = false;
						while(inMax != MaxIS[inTemp].size())
						{
							what = false;
							if(simillarity[MaxIS[inTemp][inMax]])
							{
								for(int i = 0; i < conflictArray[MaxIS[inTemp][inMax]].outTemp; i++)
									conflictArray[conflictArray[MaxIS[inTemp][inMax]].outSimilarities[i]].outTemp--;
								MaxIS[inTemp].erase(MaxIS[inTemp].begin()+inMax);
								inMax = 0;
								what = true;
								if(MaxIS[inTemp].size() == 0)
									break;
							}
							if(what == false)
								inMax++;
						}
					}
				}
				conflictArray[MaxIS[delTemp][inTmp]].finalInclude = true;
				MaxIS[delTemp].erase(MaxIS[delTemp].begin()+inTmp);
			}

			delete[] simillarity;
			simillarity = NULL;
		}
	}
	cout <<"end object function 4\n";
}
//---------------------------------------------------------------------------------------------------------------
void objectiveFunction3(conflictNode* conflictArray,vector<int> MaxIS[], int NumTemplate, int clockConstraint, string basicFile)
{
	int objNum;
	int withinTmp;

	int		performanceArea = 0;
	double	objective = -1000000.0;
	int		delTemp = 1000000000;
	bool	flagIter = false;
	double	tmpObj = 0.0;
	int i;
	cout << "begining of the objectionFunc3\n";
	for(objNum = 0; objNum < NumTemplate; objNum++)
	{//first for
		objective = -1000000.0;
		delTemp = 1000000000;
		flagIter = false;
		for(withinTmp = 0; withinTmp < NumTemplate; withinTmp++)
		{
			if(MaxIS[withinTmp].size() != 0)
			{
				tmpObj = 0.0;
				for(i = 0; i < MaxIS[withinTmp].size(); i++)
				{
					tmpObj +=  conflictArray[MaxIS[withinTmp][i]].Iteration * (conflictArray[MaxIS[withinTmp][i]].Nodes.size() *  - clockConstraint);
				}
				tmpObj = tmpObj / conflictArray[MaxIS[withinTmp][0]].Area;
				if(tmpObj > objective)
				{
					flagIter = true;
					objective = tmpObj;
					delTemp = withinTmp;
				}
			}
		}
		cout << "middle of the objectionFunc3\n";
		if(flagIter)
		{
			unsigned int inTmp = 0;
			while(!MaxIS[delTemp].empty())
			{
				for(unsigned int simi = 0; simi < conflictArray[MaxIS[delTemp][inTmp]].outSimilarities.size(); simi++)
				{
					for(int inTemp = 0; inTemp < NumTemplate; inTemp++)
					{
						int inMax = 0;
						bool what = false;
						while(inMax != MaxIS[inTemp].size())
						{
							what = false;
							if(conflictArray[MaxIS[delTemp][inTmp]].outSimilarities[simi] == MaxIS[inTemp][inMax])
							{
								MaxIS[inTemp].erase(MaxIS[inTemp].begin()+inMax);
								inMax = 0;
								what = true;
								if(MaxIS[inTemp].size() == 0)
									break;
							}
							if(what == false)
								inMax++;
						}
					}
				}
				conflictArray[MaxIS[delTemp][inTmp]].finalInclude = true;
				MaxIS[delTemp].erase(MaxIS[delTemp].begin()+inTmp);
			}
		}
	}
	cout << "end of the objectionFunc3\n";
}
//---------------------------------------------------------------------------------------------------------------
void objectiveFunction2(conflictNode* conflictArray,vector<int> MaxIS[], int NumTemplate, int clockConstraint, string basicFile)
{//begin of objectiveFunction2(number of MIS in a template ^ 1.2 * number of matches in a template)
	//1)conflictArray:			an array that has all conflict nodes
	//2)MaxIS:					MIS of each of the template
	//3)NumTemplate:			The number of Templates
	//4)basicFile:				the final file that results are written to
	//-------------------------------------------------------------------------------------------------
	int objNum;
	int withinTmp;

	int performanceArea = 0;

	for(objNum = 0; objNum < NumTemplate; objNum++)
	{//first for
		double objective = -1000000.0;
		int delTemp = 1000000000;
		bool flagIter = false;
		double tmpObj;
		unsigned int inTmp = 0;
		int i;

		for(withinTmp = 0; withinTmp < NumTemplate; withinTmp++)
		{
			if(MaxIS[withinTmp].size() != 0)
			{
				tmpObj = 0.0;
				for(i = 0; i < MaxIS[withinTmp].size(); i++)
				{
					tmpObj +=  conflictArray[MaxIS[withinTmp][i]].Iteration * (conflictArray[MaxIS[withinTmp][i]].Nodes.size() *  - clockConstraint);
				}
				if(tmpObj > objective)
				{
					flagIter = true;
					objective = tmpObj;
					delTemp = withinTmp;
				}
			}
		}
		if(flagIter)
		{
			inTmp = 0;
			while(!MaxIS[delTemp].empty())
			{
				for(unsigned int simi = 0; simi < conflictArray[MaxIS[delTemp][inTmp]].outSimilarities.size(); simi++)
				{
					for(int inTemp = 0; inTemp < NumTemplate; inTemp++)
					{
						int inMax = 0;
						bool what = false;
						while(inMax != MaxIS[inTemp].size())
						{
							what = false;
							if(conflictArray[MaxIS[delTemp][inTmp]].outSimilarities[simi] == MaxIS[inTemp][inMax])
							{
								MaxIS[inTemp].erase(MaxIS[inTemp].begin()+inMax);
								inMax = 0;
								what = true;
								if(MaxIS[inTemp].size() == 0)
									break;
							}
							if(what == false)
								inMax++;
						}
					}
				}
				conflictArray[MaxIS[delTemp][inTmp]].finalInclude = true;
				MaxIS[delTemp].erase(MaxIS[delTemp].begin()+inTmp);
			}
		}
	}
}
//------------------------------------------------------------------------------------------------
void objectiveFunction1Iter(conflictNode* conflictArray,vector<int> MaxIS[], int NumTemplate, string basicFile)
{//begin of objectiveFunction1(number of node ^ 1.2 * number of matches in a template)
	//1)conflictArray:			an array that has all conflict nodes
	//2)MaxIS:					MIS of each of the template
	//3)NumTemplate:			The number of Templates
	//4)basicFile:				the final file that results are written to
	//-------------------------------------------------------------------------------------------------
	int objNum;
	int withinTmp, i;
	long double tmpObj;
	int cntIteration;
	for(objNum = 0; objNum < NumTemplate; objNum++)
	{//first for
		long double objective = -1000000;
		int delTemp = 1000000000;
		bool flagIter = false;
		for(withinTmp = 0; withinTmp < NumTemplate; withinTmp++)
		{
			if(MaxIS[withinTmp].size() != 0)
			{
				cntIteration = 0;
				for(i = 0; i < MaxIS[withinTmp].size(); i++)
				{
					cntIteration += conflictArray[MaxIS[withinTmp][i]].Iteration;
				}
				tmpObj = pow(conflictArray[MaxIS[withinTmp][0]].Nodes.size(), 1.2) * cntIteration;
				if(tmpObj > objective)
				{
					flagIter = true;
					objective = tmpObj;
					delTemp = withinTmp;
				}
			}
		}
		if(flagIter)
		{
			unsigned int inTmp = 0;
			while(!MaxIS[delTemp].empty())
			{
				for(unsigned int simi = 0; simi < conflictArray[MaxIS[delTemp][inTmp]].outSimilarities.size(); simi++)
				{
					for(int inTemp = 0; inTemp < NumTemplate; inTemp++)
					{
						int inMax = 0;
						bool what = false;
						while(inMax != MaxIS[inTemp].size())
						{
							what = false;
							if(conflictArray[MaxIS[delTemp][inTmp]].outSimilarities[simi] == MaxIS[inTemp][inMax])
							{
								MaxIS[inTemp].erase(MaxIS[inTemp].begin()+inMax);
								inMax = 0;
								what = true;
								if(MaxIS[inTemp].size() == 0)
									break;
							}
							if(what == false)
								inMax++;
						}
					}
				}
				conflictArray[MaxIS[delTemp][inTmp]].finalInclude = true;
				MaxIS[delTemp].erase(MaxIS[delTemp].begin()+inTmp);
			}
		}
	}
}
//------------------------------------------------------------------------------------------------
void objectiveFunction1(conflictNode* conflictArray,vector<int> MaxIS[], int NumTemplate, string basicFile)
{//begin of objectiveFunction1(number of node ^ 1.2 * number of matches in a template)
	//1)conflictArray:			an array that has all conflict nodes
	//2)MaxIS:					MIS of each of the template
	//3)NumTemplate:			The number of Templates
	//4)basicFile:				the final file that results are written to
	//-------------------------------------------------------------------------------------------------
	int objNum;
	int withinTmp;
	for(objNum = 0; objNum < NumTemplate; objNum++)
	{//first for
		long double objective = -1000000;
		int delTemp = 1000000000;
		bool flagIter = false;
		for(withinTmp = 0; withinTmp < NumTemplate; withinTmp++)
		{
			if(MaxIS[withinTmp].size() != 0)
			{
				long double tmpObj = pow(conflictArray[MaxIS[withinTmp][0]].Nodes.size(), 1.2) * MaxIS[withinTmp].size();
				if(tmpObj > objective)
				{
					flagIter = true;
					objective = tmpObj;
					delTemp = withinTmp;
				}
			}
		}
		if(flagIter)
		{
			unsigned int inTmp = 0;
			while(!MaxIS[delTemp].empty())
			{
				for(unsigned int simi = 0; simi < conflictArray[MaxIS[delTemp][inTmp]].outSimilarities.size(); simi++)
				{
					for(int inTemp = 0; inTemp < NumTemplate; inTemp++)
					{
						int inMax = 0;
						bool what = false;
						while(inMax != MaxIS[inTemp].size())
						{
							what = false;
							if(conflictArray[MaxIS[delTemp][inTmp]].outSimilarities[simi] == MaxIS[inTemp][inMax])
							{
								MaxIS[inTemp].erase(MaxIS[inTemp].begin()+inMax);
								inMax = 0;
								what = true;
								if(MaxIS[inTemp].size() == 0)
									break;
							}
							if(what == false)
								inMax++;
						}
					}
				}
				conflictArray[MaxIS[delTemp][inTmp]].finalInclude = true;
				MaxIS[delTemp].erase(MaxIS[delTemp].begin()+inTmp);
			}
		}
	}
}
//-----------------------------------------------------------------------------------------------------
void objFuncSwitch(conflictNode* conflictArray, int allRepitation,vector<int> MaxIS[], int NumTemplate, string basicFile,
				  int inRegisterConstraint, int outRegisterConstraint, int ObjectiveFunction, int clockConstraint)
{
	switch(ObjectiveFunction)
	{
	case 2:
		objectiveFunction1(conflictArray, MaxIS, NumTemplate, basicFile);
		break;
	case 4:
		objectiveFunction41(conflictArray, allRepitation, MaxIS, NumTemplate, clockConstraint, basicFile, inRegisterConstraint, outRegisterConstraint);
		break;
	case 5:
		objectiveFunction42(conflictArray, allRepitation, MaxIS, NumTemplate, clockConstraint, basicFile, inRegisterConstraint, outRegisterConstraint);
		break;
	case 6:
		objectiveFunction43(conflictArray, allRepitation, MaxIS, NumTemplate, clockConstraint, basicFile, inRegisterConstraint, outRegisterConstraint);
		break;
	case 7:
		objectiveFunction44(conflictArray, allRepitation, MaxIS, NumTemplate, clockConstraint, basicFile, inRegisterConstraint, outRegisterConstraint);
		break;
	case 8:
		objectiveFunction8(conflictArray, MaxIS, NumTemplate, clockConstraint, basicFile);
		break;
	case 9:
		objectiveFunction9(conflictArray, allRepitation, MaxIS, NumTemplate, clockConstraint, basicFile, inRegisterConstraint, outRegisterConstraint);
		break;
	default:
		objectiveFunction1(conflictArray, MaxIS, NumTemplate, basicFile);
		break;
	}

}
//--------------------------------------------------------------------------------------------------------------------
void findMIS(conflictNode* conflictArray, int allRepitation, int NumTemplate, string basicFile,
			 int inRegisterConstraint, int outRegisterConstraint, int ObjectiveFunction, int clockConstraint)
{//begin of findMIS(find MIS locally in a template)
	//1)conflictArray:			an array that has all conflict nodes
	//2)allRepitation:			the number of conflict nodes	
	//3)NumTemplate:			the number of templates
	//4)basicFile:				string to save results
	//5)inRegisterConstraint	input register constraint
	//6)outRegisterConstraint	output register constraint
	//7)ObjectiveFunction		the kind of objective function
	//-------------------------------------------------------------------------------------------------
	
	cout << "begining of the findMIS\n";
	//variable definition----------------------
	vector<int> Templates[10000];
	vector<int> MaxIS[10000];//stores MIS for each template
	vector<bool> notInclude[10000];
	int minEdge = 100000000;
	int minOutEdge = 100000000;
	int tmpNum = -1;
	int deletedNode;
	unsigned int yy;
	int allRep;
	int forDelete = 0;
	bool delNodes = false;
	unsigned int intmp = 0;
	//-----------------------------------------

	//adds conflict nodes that are in a same template-------------------------------
	for(allRep = 0; allRep < allRepitation; allRep++)
		Templates[conflictArray[allRep].TemNum].push_back(conflictArray[allRep].Id);
	//------------------------------------------------------------------------------

	//loop over all templates-------------------------------------------------------
	for(allRep = 0; allRep < NumTemplate; allRep++)
	{//allRep
		while(!Templates[allRep].empty())
		{//loop over all conflict nodes that are in a same template
			for(yy = 0; yy < Templates[allRep].size(); yy++)
			{
				if(conflictArray[Templates[allRep][yy]].withTemp < minEdge)
				{//find the conflict node that has the least number of edges with other conflict node in the same template
					minEdge = conflictArray[Templates[allRep][yy]].withTemp;
					minOutEdge = conflictArray[Templates[allRep][yy]].outTemp;
					tmpNum = yy;
				}

				if(conflictArray[Templates[allRep][yy]].withTemp = minEdge)//a condition for equality
				{//find the conflict node that has the least number of edges with other conflict node that are not in the same template
					if(conflictArray[Templates[allRep][yy]].outTemp < minOutEdge)
					{
						minOutEdge = conflictArray[Templates[allRep][yy]].outTemp;
						tmpNum = yy;
					}
				}
			}
			//cout<< "after big loop\n";
			//add this node to MaxIS of this template----------
			MaxIS[allRep].push_back(Templates[allRep][tmpNum]);
			notInclude[allRep].push_back(true);
			conflictArray[Templates[allRep][tmpNum]].MISinclude = true;
			deletedNode = Templates[allRep][tmpNum];
			Templates[allRep].erase(Templates[allRep].begin()+tmpNum); // delete selected node
			forDelete = 0;
			delNodes = false;
			intmp = 0;
			while(intmp != Templates[allRep].size())
			{
				delNodes = false;
				//loop over inSimilarities node
				for(unsigned int simi = 0; simi < conflictArray[deletedNode].inSimilarities.size(); simi++)
				{
					if(Templates[allRep][intmp] == conflictArray[deletedNode].inSimilarities[simi])																					
					{//delete nodes that has edges with this node and are in the same template
						Templates[allRep].erase(Templates[allRep].begin()+ intmp);
						intmp = 0;
						delNodes = true;
						if(Templates[allRep].size() == 0)
							break;
					}
				}
				if(!delNodes)
					intmp++;
			}

			minEdge = 100000000;
			minOutEdge = 100000000;
			tmpNum = -1;
		}
		cout << "after while\n";
	}
	cout << "end of the findMIS\n";
	//call to objective function switch
	objFuncSwitch(conflictArray, allRepitation, MaxIS, NumTemplate, basicFile, inRegisterConstraint, outRegisterConstraint, ObjectiveFunction, clockConstraint); 
}
//------------------------------------------------------------------------------------------------------------------------------------------------
void updateWeight6(conflictNode* conflictArray, int clockConstraint, int allRepitation)
{//weight function that use [Merit(node) - Merit(Neighbor(node)) / (edgecount + 1)]
	int edgeCount = 0;
	double eachMerit = 0.0;
	double outSimillaritiesArea = 0.0;
	double inSimillaritiesPerformance = 0.0;
	double outSimillaritiesPerformance = 0.0;
	int i, j;
	
	bool checkTemp[10000];
	memset(checkTemp, false, 10000 * sizeof(bool));
	for(i = 0; i < allRepitation; i++)
	{
		edgeCount = 0;
		outSimillaritiesArea = 0.0;
		inSimillaritiesPerformance = 0.0;
		outSimillaritiesPerformance = 0.0;
		for(j = 0; j <  conflictArray[i].inSimilarities.size(); j++)
		{
			if(conflictArray[conflictArray[i].inSimilarities[j]].globalMIS)
				inSimillaritiesPerformance += conflictArray[conflictArray[i].inSimilarities[j]].Iteration * (conflictArray[conflictArray[i].inSimilarities[j]].Nodes.size() - clockConstraint);
		}
		for(j = 0; j <  conflictArray[i].outSimilarities.size(); j++)
		{
			if(checkTemp[conflictArray[conflictArray[i].outSimilarities[j]].TemNum] == false && conflictArray[conflictArray[i].outSimilarities[j]].globalMIS)
				outSimillaritiesArea += conflictArray[conflictArray[i].outSimilarities[j]].Area;
			checkTemp[conflictArray[conflictArray[i].outSimilarities[j]].TemNum] = true;
			if(conflictArray[conflictArray[i].outSimilarities[j]].globalMIS)
				outSimillaritiesPerformance += conflictArray[conflictArray[i].outSimilarities[j]].Iteration * (conflictArray[conflictArray[i].outSimilarities[j]].Nodes.size() - clockConstraint);
		}
		conflictArray[i].weight = (((conflictArray[i].Iteration * (conflictArray[i].Nodes.size() - clockConstraint)) - inSimillaritiesPerformance) / conflictArray[i].Area)
									- (outSimillaritiesPerformance / outSimillaritiesArea);
	}

}

//-------------------------------------------------------------------------------------------------------
void updateWeight1(conflictNode* conflictArray, int inRegister, int outRegister, int clockConstraint, int allRepitation)
{//weight function that use merit as weight
	int inConstraint = 0;
	int outConstraint = 0;
	for(int i = 0; i < allRepitation; i++)
	{
		inConstraint = conflictArray[i].inputNum > inRegister ? ceil(((conflictArray[i].inputNum) - inRegister)/(double)inRegister) : 0;
		outConstraint = conflictArray[i].outputNum > outRegister ? ceil(((conflictArray[i].outputNum) - outRegister)/(double)outRegister) : 0;
		conflictArray[i].weight = conflictArray[i].Iteration * (conflictArray[i].Nodes.size()  - clockConstraint - inConstraint - outConstraint);
	}

}
//-------------------------------------------------------------------------------------------------------
void updateWeight2(conflictNode* conflictArray,int inRegister, int outRegister, int clockConstraint, int allRepitation)
{//weight function that use [Merit(node) - Merit(Neighborhod(node))]
	double eachMerit = 0.0;
	int i, j, k;
	int inConstraint = 0;
	int outConstraint = 0;
	//updateWeight1(conflictArray, clockConstraint, allRepitation);
	for(i = 0; i < allRepitation; i++)
	{
		inConstraint = conflictArray[i].inputNum > inRegister ? ceil(((conflictArray[i].inputNum) - inRegister)/(double)inRegister) : 0;
		outConstraint = conflictArray[i].outputNum > outRegister ? ceil(((conflictArray[i].outputNum) - outRegister)/(double)outRegister) : 0;
		conflictArray[i].firstWeight = conflictArray[i].Iteration * (conflictArray[i].Nodes.size()  - clockConstraint - inConstraint - outConstraint);
	}
	for(i = 0; i < allRepitation; i++)
	{
		eachMerit = 0.0;
		for(j = 0; j < conflictArray[i].inSimilarities.size(); j++)
			if(conflictArray[conflictArray[i].inSimilarities[j]].globalMIS == true)
				eachMerit += conflictArray[conflictArray[i].inSimilarities[j]].firstWeight;
		for(j = 0; j < conflictArray[i].outSimilarities.size(); j++)
			if(conflictArray[conflictArray[i].outSimilarities[j]].globalMIS == true)
				eachMerit += conflictArray[conflictArray[i].outSimilarities[j]].firstWeight;
		conflictArray[i].weight = conflictArray[i].firstWeight - eachMerit;
	}

}
//---------------------------------------------------------------------------------------------------------
void updateWeight2_old(conflictNode* conflictArray,int inRegister, int outRegister, int clockConstraint, int allRepitation)
{//weight function that use [Merit(node) - Merit(Neighborhod(node))]
	double eachMerit = 0.0;
	int i, j, k;
	int count = 0;
	//updateWeight2first(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
	for(i = 0; i < allRepitation; i++)
	{
		eachMerit = 0.0;
		count = 0;
		for(j = 0; j < conflictArray[i].inSimilarities.size(); j++)
			if(conflictArray[conflictArray[i].inSimilarities[j]].globalMIS == true)
			{
				eachMerit += conflictArray[conflictArray[i].inSimilarities[j]].tempWeight;
				count++;
			}
		for(j = 0; j < conflictArray[i].outSimilarities.size(); j++)
			if(conflictArray[conflictArray[i].outSimilarities[j]].globalMIS == true)
			{
				eachMerit += conflictArray[conflictArray[i].outSimilarities[j]].tempWeight;
				count++;
			}
		conflictArray[i].weight =   - (eachMerit) - (conflictArray[i].tempWeight);
	}

}
//---------------------------------------------------------------------------------------------------------
void updateWeight4(conflictNode* conflictArray, int inRegister, int outRegister, int clockConstraint, int allRepitation)
{//weight function that use [Merit(node) - Merit(Neighbor(node)) / (edgecount + 1)]
	int edgeCount = 0;
	double eachMerit = 0.0;
	int i, j, k;
	int inSimillaritiesEdge = 0;
	int outSimillaritiesEdge = 0;
	updateWeight2(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
	for(i = 0; i < allRepitation; i++)
	{
		inSimillaritiesEdge = 0;
		outSimillaritiesEdge = 0;
		for(j = 0; j < conflictArray[i].inSimilarities.size(); j++)
			if(conflictArray[conflictArray[i].inSimilarities[j]].globalMIS == true)
				inSimillaritiesEdge++;
		for(j = 0; j < conflictArray[i].outSimilarities.size(); j++)
			if(conflictArray[conflictArray[i].outSimilarities[j]].globalMIS == true)
				outSimillaritiesEdge++;
		edgeCount = inSimillaritiesEdge + outSimillaritiesEdge;
		conflictArray[i].weight = (conflictArray[i].weight) / (edgeCount+1);
	}

}
//---------------------------------------------------------------------------------------------------------
void updateWeight5(conflictNode* conflictArray, int inRegister, int outRegister, int clockConstraint, int allRepitation)
{//weight function that use [Merit(node) - Merit(Neighbor(node)) / (edgecount + 1)]
	int edgeCount = 0;
	double eachMerit = 0.0;
	double outSimillaritiesArea = 0.0;
	int i, j;
	
	bool checkTemp[10000];
	memset(checkTemp, false, 10000 * sizeof(bool));

	updateWeight2(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
	for(i = 0; i < allRepitation; i++)
	{
		edgeCount = 0;
		outSimillaritiesArea = 0.0;
		for(j = 0; j <  conflictArray[i].outSimilarities.size(); j++)
		{
			if(checkTemp[conflictArray[conflictArray[i].outSimilarities[j]].TemNum] == false && conflictArray[conflictArray[i].outSimilarities[j]].globalMIS)
				outSimillaritiesArea += conflictArray[conflictArray[i].outSimilarities[j]].Area;
			checkTemp[conflictArray[conflictArray[i].outSimilarities[j]].TemNum] = true;
		}
		conflictArray[i].weight = (conflictArray[i].weight) / (conflictArray[i].Area + outSimillaritiesArea);
	}

}
//---------------------------------------------------------------------------------------------------------
void updateWeight3(conflictNode* conflictArray, int inRegister, int outRegister, int clockConstraint, int allRepitation)
{//weight function that use [Merit(node) / (edgecount + 1)]
	int edgeCount = 0;
	int i, j, k;
	int inSimillaritiesEdge = 0;
	int outSimillaritiesEdge = 0;
	updateWeight1(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
	for(i = 0; i < allRepitation; i++)
	{
		inSimillaritiesEdge = 0;
		outSimillaritiesEdge = 0;
		for(j = 0; j < conflictArray[i].inSimilarities.size(); j++)
			if(conflictArray[conflictArray[i].inSimilarities[j]].globalMIS == true)
				inSimillaritiesEdge++;
		for(j = 0; j < conflictArray[i].outSimilarities.size(); j++)
			if(conflictArray[conflictArray[i].outSimilarities[j]].globalMIS == true)
				outSimillaritiesEdge++;
		edgeCount = inSimillaritiesEdge + outSimillaritiesEdge;
		conflictArray[i].weight = conflictArray[i].weight / (edgeCount + 1);
	}

}
//------------------------------------------------------------------------------------------------------
/*void findGlobalMIS2(conflictNode* conflictArray,int allRepitation,int NumTemplate, int clockConstraint, string basicFile, int objectiveFunction)
{//begin of findGlobalMIS(find MIS globally in a template)//for our proposed algorithm
	//1)conflictArray:		an array that has all conflict nodes
	//2)allRepitation:		the number of conflict nodes	
	//3)NumTemplate:		the number of templates
	//4)basicFile:			string to save results
	//-------------------------------------------------------------------------------------------------

	//variable definition----------------------------
	int i, j, k;
	int selectedConflict;
	vector<int> finalResult; //final result
	double selectedWeight = -1000000000.0;
	int** transitionMatrix = new int*[allRepitation];
	bool outWhile = true;
	bool noNode = true;
	//-----------------------------------------------
	cout << "this is global algorithm\n";
	//variable initialization of transition matrix------------------
	for(i = 0; i < allRepitation; i++)
	{
		transitionMatrix[i] = new int[allRepitation];
		memset(transitionMatrix[i], 0, sizeof(int) * allRepitation);
	}
	//---------------------------------------------------------------

	//fill transitionMatrix with inSimilarities and outsimilarities
	for(i = 0; i < allRepitation; i++)
	{
		for(j = 0; j < conflictArray[i].inSimilarities.size(); j++)
		{
			transitionMatrix[i][j] = 1;
			transitionMatrix[j][i] = 1;
		}
				
		for(j = 0; j < conflictArray[i].outSimilarities.size(); j++)
		{
			transitionMatrix[i][j] = 1;
			transitionMatrix[j][i] = 1;
		}
		conflictArray[i].globalMIS = true;//put all node in globalMIS
		conflictArray[i].checked = false;
	}
	//---------------------------------------------------------------

	/*
	if(objectiveFunction == 1)
		updateWeight1(conflictArray, clockConstraint, allRepitation);
	else if(objectiveFunction == 2)
		updateWeight2(conflictArray, clockConstraint, allRepitation);
	else if(objectiveFunction == 3)
		updateWeight3(conflictArray, clockConstraint, allRepitation);
	else if(objectiveFunction == 4)
		updateWeight4(conflictArray, clockConstraint, allRepitation);
	else if(objectiveFunction == 5)
		updateWeight5(conflictArray, clockConstraint, allRepitation);
	else if(objectiveFunction == 6)
		updateWeight6(conflictArray, clockConstraint, allRepitation);*/

	/*double tempMerit = 0.0;
	bool loopAgain = true;

	updateWeight2(conflictArray, clockConstraint, allRepitation);

	while(outWhile)
	{
		while(loopAgain)
		{
			noNode = true;//true=no node was found with better weight, false= a node was found that has a better weight
		//loop over all conflict nodes and if globalMIS is true, the weight is checked
			selectedWeight = -1000000000.0;
			for(i = 0; i < allRepitation; i++)
			{
				if(conflictArray[i].globalMIS == true)
				{
					if(conflictArray[i].firstWeight >= selectedWeight && conflictArray[i].checked == false)
					{
						selectedWeight = conflictArray[i].firstWeight;
						selectedConflict = i;
						noNode = false;
					}
					
				}
			}
			//----------------------------------------------------------------------------
			if(noNode == false)
			{
				tempMerit = 0.0;
				conflictArray[selectedConflict].checked = true;//this node has checked
				for(k = 0; k < conflictArray[selectedConflict].inSimilarities.size(); k++)
				{
					if(conflictArray[conflictArray[selectedConflict].inSimilarities[k]].globalMIS == true)
					{
						tempMerit += conflictArray[conflictArray[selectedConflict].inSimilarities[k]].firstWeight;

					}
					if(conflictArray[conflictArray[selectedConflict].outSimilarities[k]].globalMIS == true)
					{
						tempMerit += conflictArray[conflictArray[selectedConflict].outSimilarities[k]].firstWeight;

					}

				}
				if(conflictArray[selectedConflict].weight >= tempMerit)
				{
					
					conflictArray[selectedConflict].globalMIS = false;//assign false to globalMIS of this selected node
					conflictArray[selectedConflict].finalInclude = true;
					finalResult.push_back(selectedConflict);//add selected node to finalResult

					for(i = 0; i < conflictArray[selectedConflict].outSimilarities.size(); i++)
					{//delete outSimilarities from final result
						transitionMatrix[selectedConflict][conflictArray[selectedConflict].outSimilarities[i]] = 0;
						transitionMatrix[conflictArray[selectedConflict].outSimilarities[i]][selectedConflict] = 0;
						conflictArray[conflictArray[selectedConflict].outSimilarities[i]].globalMIS = false;
					}

					for(i = 0; i < conflictArray[selectedConflict].inSimilarities.size(); i++)
					{//delete inSimilarities from final result
						transitionMatrix[selectedConflict][conflictArray[selectedConflict].inSimilarities[i]] = 0;
						transitionMatrix[conflictArray[selectedConflict].inSimilarities[i]][selectedConflict] = 0;
						conflictArray[conflictArray[selectedConflict].inSimilarities[i]].globalMIS = false;
					}
					updateWeight2(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
					for(k = 0; k < allRepitation; k++)
						conflictArray[i].checked = false;
				}
				else
					break;

			}
			else
			{
				outWhile = false;
				loopAgain = false;
			}
		}
	}
	
	//delete transitionMatrix----------------
	for (int i = 0; i < allRepitation; i++) {
		delete[] transitionMatrix[i];
		transitionMatrix[i] = NULL;
	}
	delete[] transitionMatrix;
	transitionMatrix = NULL;
	//---------------------------------------

}*/
//------------------------------------------------------------------------------------------------------
void findGlobalMIS(conflictNode* conflictArray,int allRepitation,int NumTemplate, int clockConstraint, string basicFile, int inRegister, int outRegister, int objectiveFunction)
{//begin of findGlobalMIS(find MIS globally in a template)
	//1)conflictArray:		an array that has all conflict nodes
	//2)allRepitation:		the number of conflict nodes	
	//3)NumTemplate:		the number of templates
	//4)basicFile:			string to save results
	//-------------------------------------------------------------------------------------------------

	//variable definition----------------------------
	int i, j, k;
	int selectedConflict;
	vector<int> finalResult; //final result
	double selectedWeight = -1000000000.0;
	int** transitionMatrix = new int*[allRepitation];
	bool outWhile = true;
	bool noNode = true;
	//-----------------------------------------------

	//variable initialization of transition matrix------------------
	for(i = 0; i < allRepitation; i++)
	{
		transitionMatrix[i] = new int[allRepitation];
		memset(transitionMatrix[i], 0, sizeof(int) * allRepitation);
	}
	//---------------------------------------------------------------

	//fill transitionMatrix with inSimilarities and outsimilarities(in/out are not important for global algorithm)
	for(i = 0; i < allRepitation; i++)
	{
		for(j = 0; j < conflictArray[i].inSimilarities.size(); j++)
		{
			transitionMatrix[i][j] = 1;
			transitionMatrix[j][i] = 1;
		}
				
		for(j = 0; j < conflictArray[i].outSimilarities.size(); j++)
		{
			transitionMatrix[i][j] = 1;
			transitionMatrix[j][i] = 1;
		}
		conflictArray[i].globalMIS = true;//put all node in globalMIS
		conflictArray[i].finalInclude = false;//exclude all node from final result
	}
	//---------------------------------------------------------------

	if(objectiveFunction == 1)
		updateWeight1(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
	else if(objectiveFunction == 2)
		updateWeight2(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
	else if(objectiveFunction == 3)
		updateWeight3(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
	else if(objectiveFunction == 4)
		updateWeight4(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
	else if(objectiveFunction == 5)
		updateWeight5(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
	else if(objectiveFunction == 6)
		updateWeight6(conflictArray, clockConstraint, allRepitation);

	while(outWhile)
	{
		noNode = true;//true=no node was found with better weight, false= a node was found that has a better weight
		//loop over all conflict nodes and if globalMIS is true, the weight is checked
		selectedWeight = -1000000000.0;
		for(i = 0; i < allRepitation; i++)
		{
			if(conflictArray[i].globalMIS == true)
			{
				if(conflictArray[i].weight >= selectedWeight)
				{
					selectedWeight = conflictArray[i].weight;
					selectedConflict = i;
					noNode = false;
				}
				
			}
		}
		//----------------------------------------------------------------------------
		if(noNode == false)
		{
			conflictArray[selectedConflict].globalMIS = false;//assign false to globalMIS of this selected node
			conflictArray[selectedConflict].finalInclude = true;
			finalResult.push_back(selectedConflict);//add selected node to finalResult

			for(i = 0; i < conflictArray[selectedConflict].outSimilarities.size(); i++)
			{//delete outSimilarities from final result
				transitionMatrix[selectedConflict][conflictArray[selectedConflict].outSimilarities[i]] = 0;
				transitionMatrix[conflictArray[selectedConflict].outSimilarities[i]][selectedConflict] = 0;
				conflictArray[conflictArray[selectedConflict].outSimilarities[i]].globalMIS = false;
			}

			for(i = 0; i < conflictArray[selectedConflict].inSimilarities.size(); i++)
			{//delete inSimilarities from final result
				transitionMatrix[selectedConflict][conflictArray[selectedConflict].inSimilarities[i]] = 0;
				transitionMatrix[conflictArray[selectedConflict].inSimilarities[i]][selectedConflict] = 0;
				conflictArray[conflictArray[selectedConflict].inSimilarities[i]].globalMIS = false;
			}

			if(objectiveFunction == 2)
				updateWeight2(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
			else if(objectiveFunction == 3)
				updateWeight3(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
			else if(objectiveFunction == 4)
				updateWeight4(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
			else if(objectiveFunction == 5)
				updateWeight5(conflictArray, inRegister, outRegister, clockConstraint, allRepitation);
			else if(objectiveFunction == 6)
				updateWeight6(conflictArray, clockConstraint, allRepitation);

		}
		else
			outWhile = false;
	}
	
	//delete transitionMatrix----------------
	for (int i = 0; i < allRepitation; i++) {
		delete[] transitionMatrix[i];
		transitionMatrix[i] = NULL;
	}
	delete[] transitionMatrix;
	transitionMatrix = NULL;
	//---------------------------------------

}
//----------------------------------------------------------- Rhim defines some function to programe RFU
int configure_level(ExprNode* SortedNodes, conflictNode* conflictArray,int rowofnode[15],int allparentLevel[][100],
	int inversHeigth[2][100],int i,int *violent_Row)
{
	int numOfprocessedNodes = 0;
	int usedLevel_RFU = 0;
	int current_level = 0;
	for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
	{
		if(allparentLevel[5][opI] == 0) 
		{
			numOfprocessedNodes++;
			
		}
	}
		while (numOfprocessedNodes <conflictArray[i].Nodes.size()){
			for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
			{
				if(allparentLevel[5][opI] == 0) 
				{
					//numOfprocessedNodes++;
					continue;
				}
				int ttcurentNode = allparentLevel[0][opI];
				if(allparentLevel[2][opI] == -1)
				{
					bool processed = true;
					current_level=0;
					bool parenIsHere = false;
					for(int x1=0;SortedNodes[conflictArray[i].Nodes[opI]]._NumParents > x1 ; x1++)
					{				
					
						int var1 = SortedNodes[conflictArray[i].Nodes[opI]]._ReParents[x1];
						int ttparent = SortedNodes[var1]._Id;
						for(int x2=0 ; x2 < conflictArray[i].Nodes.size() ; x2++)
						{
							if(allparentLevel[0][x2]== /*SortedNodes[var1]._Id)//*/ var1+1 && allparentLevel[5][x2]== 1)
							{
								if(allparentLevel[2][x2]== -1){processed = false;break;}
								else if(allparentLevel[1][x2] >= current_level) current_level=allparentLevel[1][x2]+1;  
							}
						}
						if (processed == false) break;
					}
					if(processed == true)
					{
						allparentLevel[1][opI]=current_level;
						allparentLevel[2][opI]=+1;
						allparentLevel[3][opI]=rowofnode[current_level];
						rowofnode[current_level]++;
						if(rowofnode[current_level] > MaxnumofRow_RFU[current_level]) *violent_Row = +1; 
						numOfprocessedNodes++;
						if( current_level > usedLevel_RFU) usedLevel_RFU = current_level;
					}
					else current_level=0;
				}
				int current_ID = SortedNodes[conflictArray[i].Nodes[opI]]._Id;
			
			}
		}
		return usedLevel_RFU;
}
int configure_output(ExprNode* SortedNodes, conflictNode* conflictArray,int rowofnode[15],int allparentLevel[][100],
	int inversHeigth[2][100],int i)
{
	int usedOutput_RFU = 0;
	for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
		{
			if(allparentLevel[5][opI] == 0)
			{
				continue;
			}
			if( SortedNodes[conflictArray[i].Nodes[opI]]._Output.size() == 0 ) 
			{
				int fff= 0; usedOutput_RFU++;
			}
			else
			{
				int tmp_output;
				for(int x5 = 0 ; x5 < SortedNodes[conflictArray[i].Nodes[opI]]._Output.size() ; x5++)
				{
					tmp_output = SortedNodes[SortedNodes[conflictArray[i].Nodes[opI]]._Output[x5]]._Id;
					for(int x2=0 ; x2 < conflictArray[i].Nodes.size() ; x2++)
					{
						if(allparentLevel[0][x2]== tmp_output && allparentLevel[5][x2] == 1)
						{
							allparentLevel[4][opI]++; 
							//CIall <<"<"<<allparentLevel[4][opI] << "," << SortedNodes[conflictArray[i].Nodes[opI]].config_level << ">";
						}
					}
				}
				if(allparentLevel[4][opI] ==-1)
				{
					//CIall <<"<-1,"<< SortedNodes[conflictArray[i].Nodes[opI]].config_level << ">";
					usedOutput_RFU++;
				}
			}

		}
	return usedOutput_RFU;
}
void configure_inversHeigth(ExprNode* SortedNodes, conflictNode* conflictArray,int rowofnode[15],int allparentLevel[][100],
	int inversHeigth[2][100],int i)
{
	int numOfprocessedNodes = 0;
	for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
	{
		if(allparentLevel[5][opI] == 0) 
		{
			numOfprocessedNodes++;
			
		}
	}
	for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
	{
		if(allparentLevel[4][opI] ==-1 && allparentLevel[5][opI] == 1)
			{
				inversHeigth[0][opI] = 0;
				inversHeigth[1][opI] = 1;
				numOfprocessedNodes++;
			}
	}
	do 
	{
		for(int opI = 0; opI < conflictArray[i] .Nodes.size(); opI++)
		{
			if(allparentLevel[5][opI] == 0) {continue;}
			if(inversHeigth[1][opI] == +1)
			{
					
				for(int x1=0;SortedNodes[conflictArray[i].Nodes[opI]]._NumParents > x1 ; x1++)
				{				
					
					int var1 = SortedNodes[conflictArray[i].Nodes[opI]]._ReParents[x1];
					for(int x2=0 ; x2 < conflictArray[i].Nodes.size() ; x2++)
					{
						if(allparentLevel[0][x2]== var1+1)
						{
							if(inversHeigth[1][x2] == -1)
							{
								inversHeigth[1][x2] = +1;
								inversHeigth[0][x2] = inversHeigth[0][opI]+1;
								numOfprocessedNodes++;
							}
							else
							{
								int a=0;
								if(inversHeigth[0][x2] < inversHeigth[0][opI] + 1)
								{										
									inversHeigth[0][x2] = inversHeigth[0][opI] + 1;
								}
							}
						}
					}						
				}					
			}			
		}
	}while (numOfprocessedNodes < conflictArray[i].Nodes.size());
}
double calc_Hlatency(ExprNode* SortedNodes, conflictNode* conflictArray,int rowofnode[15],int allparentLevel[][100],
	int inversHeigth[2][100],int i,int num_ancestors[30],int ancestors[][70],int config_level , bool RISP , int numOfConfigLevel)
{
	
	double tmp_HWlatency = 0.0;
	if(RISP == false)
	{
		
		//return conflictArray[i].HWLatency;
		if (conflictArray[i].Nodes.size() == 1)
			return 1.0;
		for(int ff = 0 ; ff <= numOfConfigLevel ; ff++)
		{
			double maxLateInRow = 0.0;
			for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
			{
				if(allparentLevel[1][opI] == ff && allparentLevel[5][opI] == 1)
				{

					if(maxLateInRow < SortedNodes[conflictArray[i].Nodes[opI]]._HwTime)
					{
						maxLateInRow = SortedNodes[conflictArray[i].Nodes[opI]]._HwTime;
					}						
				}
			}
			tmp_HWlatency=maxLateInRow + tmp_HWlatency;

		}
		return ceil(tmp_HWlatency);//((tmp_HWlatency/*+(1-0.7724)*/));
	}
	else
	{	
		return ceil( ((numOfConfigLevel+1) * ALU_RFU) + ((numOfConfigLevel+1)* MUX_RFU));
	}
	
}
double calc_Slatency(ExprNode* SortedNodes, conflictNode* conflictArray,int rowofnode[15],int allparentLevel[][100],
	int inversHeigth[2][100],int i,int num_ancestors[30],int ancestors[][70],int config_level)
{
	double SWLatency = 0.0;
	for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
	{
		if(allparentLevel[5][opI] == 1)
		     SWLatency +=1;//SortedNodes[conflictArray[i].Nodes[opI]]._SwTime;		
	}
	
	return SWLatency;
}
void fill_grandchild(ExprNode* SortedNodes, conflictNode* conflictArray,int rowofnode[15],int allparentLevel[][100],
	int inversHeigth[2][100],int i,int num_ancestors[30],int ancestors[][70],int config_level)
{
	int processed_nodes[70];
	/*for(int y=0 ; y < conflictArray[i].Nodes.size() ; y++) processed_nodes[y]=-1;*/
	int num_processed_nodes=0;
	int levelnodes[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	for( int y=0;  y < conflictArray[i].Nodes.size() ; y++)
	{
		if(allparentLevel[5][y] == 1)
			levelnodes[inversHeigth[0][y]]++;
	}
	for(int level = 0 ; level <= config_level ; level++)
	{		
		int nodesInLevel = 0;
		while(nodesInLevel < levelnodes[level])
		{
			for(int opI=0; opI < conflictArray[i].Nodes.size(); opI++)
			{
				if(inversHeigth[0][opI] == level)
				{
					if(allparentLevel[5][opI] == 0) {continue;nodesInLevel++;}
					if( SortedNodes[conflictArray[i].Nodes[opI]]._Output.size() == 0 ) 
					{
						int fff= 0; 
					}
					else
					{
						int tmp_output;
						for(int x5 = 0 ; x5 < SortedNodes[conflictArray[i].Nodes[opI]]._Output.size() ; x5++)
						{
							tmp_output = SortedNodes[SortedNodes[conflictArray[i].Nodes[opI]]._Output[x5]]._Id;
							for(int x2=0 ; x2 < conflictArray[i].Nodes.size() ; x2++)
							{
								if(allparentLevel[0][x2]== tmp_output && allparentLevel[5][x2] == 1)
								{
									//fill all grandchild --->> start section
									ancestors[opI][num_ancestors[opI]] = allparentLevel[0][x2];
									num_ancestors[opI]++;
									for(int z1=0; z1 < num_ancestors[x2] ; z1++)
									{
										bool grandchild_aval = false;
										int z2;
										for(z2 = 0 ; z2 < num_ancestors[opI] ; z2++)
										{
											if (ancestors[opI][z2] == ancestors[x2][z1])
											{
												grandchild_aval = true;
												break;
											}
										}
										if(grandchild_aval == false)
										{
											ancestors[opI][num_ancestors[opI]] = ancestors[x2][z1];
											num_ancestors[opI]++;
										}
									}							
									// fill all grandchild << ---- end section 									
								}
							}
						}						
					}
					nodesInLevel++;
				}
				
			}
		}
	}
}
void fill_ancestors(ExprNode* SortedNodes, conflictNode* conflictArray,int rowofnode[15],int allparentLevel[][100],
	int inversHeigth[2][100],int i,int num_ancestors[30],int ancestors[][70],int config_level)
{
	int processed_nodes[70];
	/*for(int y=0 ; y < conflictArray[i].Nodes.size() ; y++) processed_nodes[y]=-1;*/
	int num_processed_nodes=0;
	int levelnodes[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	for( int y=0;  y < conflictArray[i].Nodes.size(); y++)
	{
		if(allparentLevel[5][y] == 1)
			levelnodes[allparentLevel[1][y]]++;
	}
	for(int level = 0 ; level <= config_level ; level++)
	{		
		int nodesInLevel = 0;
		while(nodesInLevel < levelnodes[level])
		{
			for(int opI=0; opI < conflictArray[i].Nodes.size(); opI++)
			{
				if(allparentLevel[1][opI] == level)
				{
					int tmp_numPrimInput = 0;
					if(allparentLevel[5][opI] == 0) {continue;nodesInLevel++;}
					for(int x1=0;SortedNodes[conflictArray[i].Nodes[opI]]._NumParents > x1 ; x1++)
					{
						int var1 = SortedNodes[conflictArray[i].Nodes[opI]]._Parents[x1];
						//var1++;
						if( SortedNodes[var1]._Op.compare("IN") == 0 )
						{							
							int diff = 0;
						}
						else if( SortedNodes[var1]._Op.compare("CONS") == 0 )
						{
							int fff=0;//do nothing
						}						
						else
						{
							bool AvaliableRFU = false;
							for(int x2=0 ; x2 < conflictArray[i].Nodes.size() ; x2++)
							{
								if(allparentLevel[0][x2]== SortedNodes[var1]._Id && allparentLevel[5][opI] == 1)
								{
									AvaliableRFU = true; 
									//fill all ancestor --->> start section
									ancestors[opI][num_ancestors[opI]] = allparentLevel[0][x2];
									num_ancestors[opI]++;
									for(int z1=0; z1 < num_ancestors[x2] ; z1++)
									{
										bool ancestor_aval = false;
										int z2;
										for(z2 = 0 ; z2 < num_ancestors[opI] ; z2++)
										{
											if (ancestors[opI][z2] == ancestors[x2][z1])
											{
												ancestor_aval = true;
												break;
											}
										}
										if(ancestor_aval == false)
										{
											ancestors[opI][num_ancestors[opI]] = ancestors[x2][z1];
											num_ancestors[opI]++;
										}
									}							
									// fill all ancestor << ---- end section
									break;
								}
							}							
						}				
					}
					nodesInLevel++;
				}
				
			}
		}
	}
	
}
int configure_input(ExprNode* SortedNodes, conflictNode* conflictArray,int rowofnode[15],int allparentLevel[][100],
	int inversHeigth[2][100],int i,int num_ancestors[30],int ancestors[][70])
{
	int usedInput_RFU=0;
	int inputsArray[100];	
		int inputsArray_noReg[100];
		int num_primerInputs = 0;
		int num_nonPrimeInputs = 0;
		for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
		{
			int tmp_numPrimInput = 0;
			if(allparentLevel[5][opI] == 0) continue;
			for(int x1=0;SortedNodes[conflictArray[i].Nodes[opI]]._NumParents > x1 ; x1++)
			{
				int var1 = SortedNodes[conflictArray[i].Nodes[opI]]._Parents[x1];
				//var1++;
				if( SortedNodes[var1]._Op.compare("IN") == 0 )
				{
					bool available = false;
					allparentLevel[6][opI]++;
					for( int gh = 0; gh < num_primerInputs ; gh++) if(SortedNodes[var1].regFile == inputsArray[gh])
					{
						available = true; break;
					}
					if(available == false){	usedInput_RFU++;inputsArray[num_primerInputs]=SortedNodes[var1].regFile;num_primerInputs++;}
					
				}
				else if( SortedNodes[var1]._Op.compare("CONS") == 0 )
				{
					int fff=0;//do nothing
				}
				
				else
				{
					bool AvaliableRFU = false;
					for(int x2=0 ; x2 < conflictArray[i].Nodes.size() ; x2++)
					{
						if(allparentLevel[0][x2]== SortedNodes[var1]._Id && allparentLevel[5][opI] == 1)
						{
							AvaliableRFU = true; 
							 //fill all ancestor --->> start section
							/*ancestors[opI][num_ancestors[opI]] = allparentLevel[0][x2];
							num_ancestors[opI]++;
							for(int z1=0; z1 < num_ancestors[x2] ; z1++)
							{
								bool ancestor_aval = false;
								int z2;
								for(z2 = 0 ; z2 < num_ancestors[opI] ; z2++)
								{
									if (ancestors[opI][z2] == ancestors[x2][z1])
									{
										ancestor_aval = true;
										break;
									}
								}
								if(ancestor_aval == false)
								{
									ancestors[opI][num_ancestors[opI]] = ancestors[x2][z1];
									num_ancestors[opI]++;
								}
							}*/
							
							// fill all ancestor << ---- end section
							break;
						}
					}
					if( AvaliableRFU == false)
					{						
						allparentLevel[6][opI]++;
						bool available= false;
						for( int gh = 0; gh < num_nonPrimeInputs ; gh++)
							if(SortedNodes[var1]._Id == inputsArray_noReg[gh])
							{
								available = true; break;
							}
						if(available == false)
						{
							usedInput_RFU++;inputsArray_noReg[num_nonPrimeInputs]=SortedNodes[var1]._Id;num_nonPrimeInputs++;
						}							
					}
				}				
			}
			
		}
		
		for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
		{
			allparentLevel[4][opI] = -1;
		}
		return usedInput_RFU;
}
int input_limitation(ExprNode* SortedNodes, conflictNode* conflictArray,int rowofnode[15],int allparentLevel[][100],
	int inversHeigth[][100],int i)
{
	int tmp_value = 1000;
	int selected_node = -1;
	for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
	{
		if( allparentLevel[5][opI] == 1)
		{

			if( tmp_value > inversHeigth[1][opI]) 
			{
				tmp_value = inversHeigth[1][opI];
				selected_node = opI;
			}
			else if (tmp_value == inversHeigth[1][opI])
			{
				if( allparentLevel[1][opI] < allparentLevel[1][selected_node] )
				{
					tmp_value = inversHeigth[1][opI];
					selected_node = opI;
				}
				else if( allparentLevel[1][opI] == allparentLevel[1][selected_node] )
				{
					if(allparentLevel[4][opI] < allparentLevel[4][selected_node])
					{
						tmp_value = inversHeigth[1][opI];
						selected_node = opI;
					}
				}
			}
		}
	}
	return selected_node;
}
int output_limitation(ExprNode* SortedNodes, conflictNode* conflictArray,int rowofnode[15],int allparentLevel[][100],
	int inversHeigth[][100],int i)
{
	int tmp_value = 1000;
	int selected_node = -1;
	for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
	{
		if( allparentLevel[5][opI] == 1 && allparentLevel[4][opI] == -1)
		{
			if(tmp_value > allparentLevel[1][opI])
			{
				tmp_value = allparentLevel[1][opI];
				selected_node = opI;
			}
		}

	}
	return selected_node;
}
void level_limitation(ExprNode* SortedNodes, conflictNode* conflictArray,int rowofnode[15],int allparentLevel[][100],
	int inversHeigth[][100],int i,int config_level)
{
	int tmp_value = 1000;
	int selected_node = -1;
	for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
	{
		if( allparentLevel[5][opI] == 1 && allparentLevel[1][opI] == config_level)
		{
			allparentLevel[5][opI] = 0;
			return;
		}
	}
}
int configur_row(ExprNode* SortedNodes, conflictNode* conflictArray,int rowofnode[15],int allparentLevel[][100],
	int inversHeigth[2][100],int i,int num_ancestors[30],int ancestors[][70],int num_grandchild[30],int grandchild[][70],int config_level)
{
	return 0;
}

void intial_variable(ExprNode* SortedNodes, conflictNode* conflictArray,int rowofnode[15],int allparentLevel[][100],
	int inversHeigth[][100],int i,int num_ancestors[30])
{
	for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
		{
			allparentLevel[0][opI] = SortedNodes[conflictArray[i].Nodes[opI]]._Id;// id in basic block extracted 
			allparentLevel[1][opI] = 0; // configuration level of RFU 
			allparentLevel[2][opI] = -1; // Is it investigated for parent in avaliable nodes
			allparentLevel[3][opI] = 0; // configuration row of foe each level in RFU
			allparentLevel[4][opI] = -1; // identify the node have primery outputs or not
			//allparentLevel[5][opI] = +1; // the node have permision to appear in RFU
			allparentLevel[6][opI] = 0; // number of prime inputs
			allparentLevel[7][opI] = 0; // number of all ancestors
			allparentLevel[8][opI] = 0; // number of all grandchild
			num_ancestors[opI] = 0;
			inversHeigth[0][opI] = 0;
			inversHeigth[1][opI] = -1;
			/*if(SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("REL")==0)
				allparentLevel[5][opI] = 0;*/

		}
}
double calculat_area4Risp()
{
	double temp_area = 0.0;
	for( int  k = 0 ; k < MaxNumoflevel_RFU ; k++)
	{
		//                                           ( add   +  sub    +  4 *or      + eqt    +  2 * shr    + rel  + 2 * xor    + multiplexer
		temp_area = temp_area + MaxnumofRow_RFU[k] * (0.0034 + 0.0034  +  4 * 0.0003 + 0.0007 +  2 * 0.0027 + 0.004+ 2 * 0.0005 + 4 * 0.0003 );
	}
	return temp_area;
}
//---------------------------------------------------------------------------------------------------------
void makeConflictGraph(int lenCI, string basicFile, int DFGSize, ExprNode* SortedNodes, ExprNode* Nodes, 
					   TInOutNum T, int inRegisterConstraint, int outRegisterConstraint, bool localAlgorithm, int objectiveFunction,
					   int clockConstraint, bool firstIteration)
{//begin of makeConflictGraph(build Conflict Graph)
	//1)lenCI:				the number of found CI
	//2)basicFile:			the number of conflict nodes	
	//3)DFGSize:			size of input DFG
	//4)SortedNodes:		an array that contains node's information that are in topological order
	//5)Nodes:				an array that contains node's information that are in topological order
	//6)T:					input and output constraint
	//-------------------------------------------------------------------------------------------------

	//calculate the time of our algorithm
	precision_timer selectionTimer;
	selectionTimer.start();
	//-----------------------------------

	//variable definition-----------------------------------------
	int i, j; //variables that will be used in loops
	int inTemp;//loops betweeen each CI
	int eachMatch;
	int allRepitation = 0; //number of all found CIs	
	int conflictNumber = 0; //index for conflict array
	int NumTemplate = 0;
	int conflictid = 0; //ID of each item in conflict array
	//------------------------------------------------------------
	
	//calculate the number of CIs that don't violate input or output constraint
	for(i = 0; i < lenCI; i++)
	{
		if(CIarray[i]->inputNum <= T.inNum && CIarray[i]->outputNum <= T.outNum)
			allRepitation += CIarray[i]->NumRep;
	}
	conflictNode* conflictArray = new conflictNode[allRepitation];
	//--------------------------------------------------------------------------

	//fill items in conflictArray-----------------------------------------------
	double area_RFU = calculat_area4Risp();
	for(i = 0; i < lenCI; i++)
	{
		if(CIarray[i]->inputNum <= T.inNum && CIarray[i]->outputNum <= T.outNum)//check input and output constraint
		{
			for(j = 0; j < CIarray[i]->numNodes; j++)//fill nodes
				conflictArray[conflictNumber].Nodes.push_back(CIarray[i]->Nodes[j]);
			conflictArray[conflictNumber].TemNum = i; //fill template number
			conflictArray[conflictNumber].Merit = CIarray[i]->selfMerit;//fill selfmerit
			conflictArray[conflictNumber].oldMerit = CIarray[i]->oldMerit;//fill oldmerit
			conflictArray[conflictNumber].inputNum = CIarray[i]->inputNum;//fill number of input
			conflictArray[conflictNumber].outputNum = CIarray[i]->outputNum;//fill number of output
			conflictArray[conflictNumber].Area = CIarray[i]->area;//fill area
			conflictArray[conflictNumber].Power = CIarray[i]->power;//fill power
			conflictArray[conflictNumber].HWLatency = CIarray[i]->HWLatency;//fill HWlatency
			conflictArray[conflictNumber].Iteration = CIarray[i]->Iteration;//fill iteration

			//fill with default variable
			conflictArray[conflictNumber].CommonNodes = 0;
			conflictArray[conflictNumber].outTemp = 0;
			conflictArray[conflictNumber].MISinclude = false;
			conflictArray[conflictNumber].finalInclude = false;
			conflictArray[conflictNumber].withTemp = 0;
			conflictArray[conflictNumber].Id = conflictid;//fill the id of conflict node
			conflictid++;//increment the conflictid by one
			conflictNumber++;//increment the conflict number by one

			//fill other similliar nodes that are in this template
			for(inTemp = 0; inTemp < CIarray[i]->NumRep - 1; inTemp++)
			{
				//fill opertion
				for(eachMatch = inTemp * CIarray[i]->numNodes; eachMatch < CIarray[i]->numNodes + inTemp * CIarray[i]->numNodes; eachMatch++)
					conflictArray[conflictNumber].Nodes.push_back(CIarray[i]->otherOp[eachMatch]);
				conflictArray[conflictNumber].Merit = CIarray[i]->otherMerit[inTemp];//fill merit
				conflictArray[conflictNumber].oldMerit = CIarray[i]->oldotherMerit[inTemp];//fill oldmerit
				conflictArray[conflictNumber].inputNum = CIarray[i]->inputNum;//fill input number
				conflictArray[conflictNumber].Area = CIarray[i]->area;//fill area
				conflictArray[conflictNumber].Power = CIarray[i]->power;//fill power
				conflictArray[conflictNumber].outputNum = CIarray[i]->outputNum;//fill output number
				conflictArray[conflictNumber].HWLatency = CIarray[i]->HWLatency;//fill HW latency
				conflictArray[conflictNumber].Iteration = CIarray[i]->otherIteration[inTemp];//fill 
				conflictArray[conflictNumber].TemNum = i;//fill template number
				conflictArray[conflictNumber].Id = conflictid;//fill id
				conflictid++;//increment conflict id by one
				conflictArray[conflictNumber].CommonNodes = 0;
				conflictArray[conflictNumber].MISinclude = false;
				conflictArray[conflictNumber].finalInclude = false;
				conflictArray[conflictNumber].withTemp = 0;
				conflictArray[conflictNumber].outTemp = 0;
				conflictNumber++;
			}
		}
	}

	NumTemplate = i;

	cout << "all Repitation: " << allRepitation << '\n';
	//fill common nodes and (in/out)Simillarities
	for(int vi = 0; vi < allRepitation; vi++)
	{
		for(int vj = vi + 1; vj < allRepitation; vj++)
		{
			//cout << "vi-vj: " << vi << "-" << vj << "allRepitation: " << allRepitation <<'\n';
			//check wether there is any simillarities
			if(findSimilarities(conflictArray[vi].Nodes, conflictArray[vj].Nodes))
			{
				conflictArray[vi].CommonNodes++;
				conflictArray[vj].CommonNodes++;
				//fill insimillarities
				if(conflictArray[vi].TemNum == conflictArray[vj].TemNum)
				{
					conflictArray[vi].inSimilarities.push_back(vj);
					conflictArray[vj].inSimilarities.push_back(vi);
					conflictArray[vi].withTemp++;
					conflictArray[vj].withTemp++;
				}
				//fill outSimillarities
				else
				{
					conflictArray[vi].outSimilarities.push_back(vj);
					conflictArray[vj].outSimilarities.push_back(vi);
					conflictArray[vi].outTemp++;
					conflictArray[vj].outTemp++;
				}
			}
		}
	}

	cout<< "this is the end!!!" << '\n';

	if(localAlgorithm)
	{
		cout<< "this is the next!!!" << '\n';
		if (objectiveFunction == 0)
			maxMIS0(conflictArray, allRepitation, inRegisterConstraint, outRegisterConstraint, clockConstraint); //with area
		else if (objectiveFunction == 1)
			maxMIS1(conflictArray, allRepitation, inRegisterConstraint, outRegisterConstraint, clockConstraint);//without area
		else{
			cout << "before find MIS\n";
			findMIS(conflictArray, allRepitation, NumTemplate, basicFile, inRegisterConstraint, outRegisterConstraint, objectiveFunction, clockConstraint);
		}
		
	}
	else
	{
		//if(objectiveFunction == 2)
		//	findGlobalMIS2(conflictArray, allRepitation, NumTemplate, clockConstraint, basicFile, objectiveFunction); //last input is a switch
		//else
			findGlobalMIS(conflictArray, allRepitation, NumTemplate, clockConstraint, basicFile, inRegisterConstraint, outRegisterConstraint, objectiveFunction); //last input is a switch
	}
	selectionTimer.stop();
	//    ---===----------------------------------
	Operations* allOperations[20];
	int rex=readDelayArea("D:\\education\\thesis_Msc\\allcode_v2\\CI_reconfig\\CI_extract_reconfig\\areadelay.txt", allOperations);
	
	/*ofstream CIall;
	CIall.open("CIselect.txt");*/
	for(int i = 0; i < allRepitation ; i++)
	{
		if( i == 1000 ) break;
		double	_Delay_tmp=0;
		double	_Area_tmp=0;
		double	_DynamicPower_tmp=0;
		int rowofnode[20];
		int allparentLevel[9][100];

		int inversHeigth[2][100];
		int ancestors[30][70];
		int grandchild[30][70];
		int num_grandchild[30];
		int num_ancestors[30];
		int poiter_allparent=0;
		int usedRow_RFU, usedLevel_RFU, usedInput_RFU, usedOutput_RFU;
		usedRow_RFU= 0; usedLevel_RFU= 0; usedInput_RFU= 0; usedOutput_RFU = 0;
		for(int x2=0 ; x2< 20 ; x2++) rowofnode[x2]=0;
		int current_row,current_level; current_row = 0 ; current_level = 0;
		int current_parent = 0;
		//--------------------- Sort base on parent ----------------------
		for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
		{
			allparentLevel[0][opI] = SortedNodes[conflictArray[i].Nodes[opI]]._Id;// id in basic block extracted 
			allparentLevel[1][opI] = 0; // configuration level of RFU 
			allparentLevel[2][opI] = -1; // Is it investigated for parent in avaliable nodes
			allparentLevel[3][opI] = 0; // configuration row of foe each level in RFU
			allparentLevel[4][opI] = -1; // identify the node have primery outputs or not
			allparentLevel[5][opI] = +1; // the node have permision to appear in RFU
			allparentLevel[6][opI] = 0; // number of prime inputs
			allparentLevel[7][opI] = 0; // number of all ancestors
			allparentLevel[8][opI] = 0; // number of all grandchild
			num_ancestors[opI] = 0;
			num_grandchild[opI] = 0;
			inversHeigth[0][opI] = 0;
			inversHeigth[1][opI] = -1;
			/*if(SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("REL")==0)
				allparentLevel[5][opI] = 0;*/

		}
		//========================================= configuration Level : start section ==================================
		int violent_Row = -1;
		usedLevel_RFU = configure_level(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,&violent_Row);
		//=============================== configuration level : end section ========================================
		usedOutput_RFU = configure_output(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i);		
		// ============================== Invers Heigth : Start section=============================================
		configure_inversHeigth(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i);		
		// ============================== Invers Heigth : End section=============================================
		usedInput_RFU = configure_input(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_ancestors,ancestors);	
		fill_ancestors(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_ancestors,ancestors,usedLevel_RFU);	
		fill_grandchild(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_grandchild,grandchild,usedLevel_RFU);
		
		/*for(int xy = 0; xy <MaxNumoflevel_RFU ; xy++)
		{
			if(rowofnode[xy] > MaxnumofRow_RFU[xy])
			{
				violent_Row = 1;
			}
		}*/
		//----------------------------------------- print configuration limitation
		/*CIall <<"number of level: " << usedLevel_RFU+1 <<" row:<";
		for(int ux= 0; ux <=usedLevel_RFU ; ux++) 
			CIall << rowofnode[ux] << ","; 
		CIall<<"> Input: "<< usedInput_RFU << " output: " << usedOutput_RFU <<"|"; 
		//-----------------------------------------*/
		//************************************************************************************************ 
		//*																							     *
		//*									identify nodes can be in RFU : Start section			     *
		//*																							     *
		//************************************************************************************************
		if( RISP == true)
		{
			int violate_row[20][2];
			for(int kk =0 ; kk < MaxNumoflevel_RFU ; kk++) violate_row[kk][0] = -1;
			for(int kk =0; kk < MaxNumoflevel_RFU  ; kk++ )
			{
				while(rowofnode[kk] > MaxnumofRow_RFU[kk])
				{
				//-------------------- find  best node in the specified 
					int temp;
					for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
					{
						if(allparentLevel[1][opI] == kk && allparentLevel[5][opI] ==1)
						{
							if(num_ancestors[opI] > num_grandchild[opI])
								temp = num_grandchild[opI];
							else
								temp = num_ancestors[opI];					
							if(violate_row[kk][0] == -1)
							{
								violate_row[kk][0] = opI;
								violate_row[kk][1] = temp;
							}
							else
							{
								if(violate_row[kk][1] > temp)
								{
									violate_row[kk][0] = opI;
									violate_row[kk][1] = temp;
								}
							}
						}
					}
				//------------------------ delete ancestor or grandchild of node
					rowofnode[kk]--;
					allparentLevel[5][violate_row[kk][0]] = 0;
					if(num_grandchild[violate_row[kk][0]] == violate_row[kk][1])
					{
						for(int hh = 0 ; hh < num_grandchild[violate_row[kk][0]] ; hh++)
						{
							for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
							{
								if(allparentLevel[5][opI] ==1 && grandchild[violate_row[kk][0]][hh]== allparentLevel[0][opI])
								{
									allparentLevel[5][opI] = 0;
									break;
								}
							}
						}
					}
					else
					{
						for(int hh = 0 ; hh <num_ancestors[violate_row[kk][0]] ; hh++)
						{
							for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
							{
								if(allparentLevel[5][opI] == 1 && ancestors[violate_row[kk][0]][hh]== allparentLevel[0][opI])
								{
									allparentLevel[5][opI] = 0;
									break;
								}
							}
						}
					}
					for(int kk =0 ; kk < MaxNumoflevel_RFU ; kk++) violate_row[kk][0] = -1;
					for(int x2=0 ; x2< 20 ; x2++) rowofnode[x2]=0;
					intial_variable(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_ancestors);
					 //************************
					usedLevel_RFU = configure_level(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,&violent_Row);
					//=============================== configuration level : end section ========================================
					usedOutput_RFU = configure_output(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i);		
					// ============================== Invers Heigth : Start section=============================================
					configure_inversHeigth(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i);		
					// ============================== Invers Heigth : End section=============================================
					usedInput_RFU = configure_input(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_ancestors,ancestors);	
					fill_ancestors(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_ancestors,ancestors,usedLevel_RFU);	
					fill_grandchild(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_grandchild,grandchild,usedLevel_RFU);
					//*************************
				}
			
			}
			while (usedLevel_RFU+1 > MaxNumoflevel_RFU)
			{
				level_limitation(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,usedLevel_RFU);			
				for(int x2=0 ; x2< 20 ; x2++) rowofnode[x2]=0;
				intial_variable(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_ancestors);
				usedLevel_RFU = configure_level(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,&violent_Row);
				//=============================== configuration level : end section ========================================
				usedOutput_RFU = configure_output(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i);		
				// ============================== Invers Heigth : Start section=============================================
				configure_inversHeigth(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i);		
				// ============================== Invers Heigth : End section=============================================
				usedInput_RFU = configure_input(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_ancestors,ancestors);
			}
			while(usedInput_RFU > MaxNumofinput_RFU || usedOutput_RFU > MaxNumofoutput_RFU)
			{
				while (usedInput_RFU > MaxNumofinput_RFU)
				{
					int selected_node = input_limitation(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i);
					allparentLevel[5][selected_node] = 0;
					if( allparentLevel[6][selected_node] > 1) usedInput_RFU--;
					for(int x2=0 ; x2< 20 ; x2++) rowofnode[x2]=0;
					intial_variable(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_ancestors);
					usedLevel_RFU = configure_level(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,&violent_Row);
					//=============================== configuration level : end section ========================================
					usedOutput_RFU = configure_output(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i);		
					// ============================== Invers Heigth : Start section=============================================
					configure_inversHeigth(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i);		
					// ============================== Invers Heigth : End section=============================================
					usedInput_RFU = configure_input(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_ancestors,ancestors);			
				}
				while (usedOutput_RFU > MaxNumofoutput_RFU)
				{
					int selected_node = output_limitation(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i);
					allparentLevel[5][selected_node] = 0;			
					for(int x2=0 ; x2< 20 ; x2++) rowofnode[x2]=0;
					intial_variable(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_ancestors);
					usedLevel_RFU = configure_level(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,&violent_Row);
					//=============================== configuration level : end section ========================================
					usedOutput_RFU = configure_output(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i);		
					// ============================== Invers Heigth : Start section=============================================
					configure_inversHeigth(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i);		
					// ============================== Invers Heigth : End section=============================================
					usedInput_RFU = configure_input(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_ancestors,ancestors);
				}
			}
		}
		
		//+++++++++++++++++++++++++++++++++ identify nodes can be in RFU : End section  ++++++++++++++++++
		//----------------------------------------- print configuration limitation
		/*CIall <<"number of level: " << usedLevel_RFU+1 <<" row:<";
		for(int ux= 0; ux <=usedLevel_RFU ; ux++) 
			CIall << rowofnode[ux] << ","; 
		CIall<<"> Input: "<< usedInput_RFU << " output: " << usedOutput_RFU <<"|";		
		for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
		{	
			if(allparentLevel[5][opI] == 0) continue;
			CIall <<*/ /*conflictArray[i].Nodes[opI] <<*/ /*"(" << SortedNodes[conflictArray[i].Nodes[opI]]._Op;			
			if(SortedNodes[conflictArray[i].Nodes[opI]].hasCons)
				CIall << "_C";
			SortedNodes[conflictArray[i].Nodes[opI]].config_level=allparentLevel[1][opI];
			SortedNodes[conflictArray[i].Nodes[opI]].config_row= allparentLevel[3][opI];//rowofnode[allparentLevel[1][opI]];
			//----------------------------------------- explore output of nodes : Start
			CIall << "<invers heigth :" << inversHeigth[0][opI] << ">";
			CIall << " output:<";
			if( SortedNodes[conflictArray[i].Nodes[opI]]._Output.size() == 0 ) 
				{
					CIall <<"<P-1,"<< SortedNodes[conflictArray[i].Nodes[opI]].config_level << ">";
				}
				else
				{
					int tmp_output;
					for(int x5 = 0 ; x5 < SortedNodes[conflictArray[i].Nodes[opI]]._Output.size() ; x5++)
					{
						tmp_output = SortedNodes[SortedNodes[conflictArray[i].Nodes[opI]]._Output[x5]]._Id;
						for(int x2=0 ; x2 < conflictArray[i].Nodes.size() ; x2++)
						{
							if(allparentLevel[0][x2]== tmp_output)
							{
								allparentLevel[4][opI]++; 
								CIall <<"<"<<allparentLevel[4][opI] << "," << SortedNodes[conflictArray[i].Nodes[opI]].config_level << ">";
							}
						}
					}
					if(allparentLevel[4][opI] ==-1)
					{
						CIall <<"<-1,"<< SortedNodes[conflictArray[i].Nodes[opI]].config_level << ">";
					}
				}

			
			CIall << ">";
			//----------------------------------------- explore output of nodes : End
			//==================== clean this section : start
			int killme = SortedNodes[conflictArray[i].Nodes[opI]]._Id;
			CIall << "<ID:" << killme<<",Par:";
			for(int x1=0;SortedNodes[conflictArray[i].Nodes[opI]]._NumParents > x1 ; x1++)
			{				
				int var1 = SortedNodes[conflictArray[i].Nodes[opI]]._ReParents[x1];
				CIall<<var1+1 <<",";
			}
			CIall <<"> ";
			//==================== clean this section : end
			//---------------------------------------------------------------------
			
			//rowofnode[allparentLevel[1][opI]]=rowofnode[allparentLevel[1][opI]]+1 ;
			CIall << "<" <<allparentLevel[1][opI] <<"," <<allparentLevel[3][opI] *//*rowofnode[allparentLevel[1][opI]]-1*//*<< ">";
			CIall << ",inputs:<";
			for(int x1=0;SortedNodes[conflictArray[i].Nodes[opI]]._NumParents > x1 ; x1++)
			{
				int var1 = SortedNodes[conflictArray[i].Nodes[opI]]._Parents[x1];
				//var1++;
				if( SortedNodes[var1]._Op.compare("IN") == 0 )
				{
					CIall << "R" << SortedNodes[var1].regFile;
				}
				else if( SortedNodes[var1]._Op.compare("CONS") == 0 )
				{
					CIall << "CONS";
				}
				
				else
				{
					bool AvaliableRFU = false;
					for(int x2=0 ; x2 < conflictArray[i].Nodes.size() ; x2++)
					{
						if(allparentLevel[0][x2]== SortedNodes[var1]._Id && allparentLevel[5][x2] == 1)
						{
							AvaliableRFU = true; 
							CIall << ",<" << allparentLevel[1][x2] <<"," << allparentLevel[3][x2]<< ">";
							break;
						}
					}
					if( AvaliableRFU == false)
					{
						CIall << "R" <<( SortedNodes[var1+1]._Id)%32; // it's not incorrect
					}
				}

				CIall <<",";
				}
			CIall <<">";
			//-----------------------------------------------------------------------------
			
			CIall <<")";*/
		//without
		// without configuration bits
        double HWLat = calc_Hlatency(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_grandchild,grandchild,usedLevel_RFU,RISP,usedLevel_RFU);
		double SWLat = calc_Slatency(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_grandchild,grandchild,usedLevel_RFU);
		if ( SWLat/HWLat != 1  && HWLat <= clock_number) 
		{
			for(int opI = 0; opI < conflictArray[i].Nodes.size(); opI++)
			{
				CIall << /*conflictArray[i].Nodes[opI] <<*/ "(" << SortedNodes[conflictArray[i].Nodes[opI]]._Op;
			
				if(SortedNodes[conflictArray[i].Nodes[opI]].hasCons)
					CIall << "_C";
				CIall <<")";
				// without configuration bits
				int operationIndex=-1; 
					if(SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("SHR") == 0)
					{
						operationIndex = 11;
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("SHL") == 0)
					{
						operationIndex = 12;
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("ADD") == 0)
					{
						operationIndex = 0;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("REL") == 0)
					{
						operationIndex = 13;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("MIN") == 0)
					{
						operationIndex = 14;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("OR") == 0)
					{
						operationIndex = 5;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("ORC") == 0)
					{
						operationIndex = 6;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("AND") == 0)
					{
						operationIndex = 2;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("SUB") == 0)
					{
						operationIndex = 1;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("EQT") == 0)
					{
						operationIndex = 7;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("SLCT") == 0)
					{
						operationIndex = 15;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("XOR") == 0)
					{
						operationIndex = 16;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("SHADD") == 0)
					{
						operationIndex = 10;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("NAND") == 0)
					{
						operationIndex = 3;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("NOR") == 0)
					{
						operationIndex = 4;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("XNOR") == 0)
					{
						operationIndex = 17;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("SZXT") == 0)
					{
						operationIndex = 18;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("MUL") == 0)
					{
						operationIndex = 8;			
					}
					else if (SortedNodes[conflictArray[i].Nodes[opI]]._Op.compare("IMUL") == 0)
					{
						operationIndex = 9;			
					}
					if(operationIndex!=-1)
					{
						_Delay_tmp=_Delay_tmp+allOperations[operationIndex]->_Delay;
						_Area_tmp=_Area_tmp+allOperations[operationIndex]->_Area;
						_DynamicPower_tmp=_DynamicPower_tmp+allOperations[operationIndex]->_DynamicPower;
					}
			
			}
			HWLat = calc_Hlatency(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_grandchild,grandchild,usedLevel_RFU,RISP,usedLevel_RFU);
			SWLat = calc_Slatency(SortedNodes,conflictArray,rowofnode,allparentLevel,inversHeigth,i,num_grandchild,grandchild,usedLevel_RFU);
			if ( HWLat == 0 )
			{
				HWLat = 1;
				SWLat = 1;
			}
			CIall <<"\n";
			if ( SWLat/HWLat != 1  && HWLat <= clock_number) 
				//continue;
			{
				RFU_profile4size << usedInput_RFU << "\t" << usedOutput_RFU << "\t" << usedLevel_RFU+1 <<"\t" << HWLat << "\t" << /*conflictArray[i].Iteration*(SWLat-HWLat)*/(SWLat/HWLat)<< "\t<\t";
				for( int yy = 0 ; yy <= usedLevel_RFU ; yy++)
					RFU_profile4size << rowofnode[yy] << "\t";
				RFU_profile4size << ">\t"<< endl;		
			}
			double area = 0.0;
			if ( RISP== true)
				area = calculat_area4Risp();
			else
				area = conflictArray[i].Area;
			CIall << conflictArray[i].Merit<<"\t"<< conflictArray[i].Iteration <<"\t" <<area <<"\t"<< conflictArray[i].Iteration*(SWLat-HWLat/*_Delay_tmp-conflictArray[i].HWLatency*/)<<"\t";
			CIall <<conflictArray[i].Iteration*(/*_DynamicPower_tmp-*/conflictArray[i].Power);
			CIall <<"\n";
		}
		
	}
	
	//    -*-*-*-*-*-*-*-*-**-*-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*
	printResult(basicFile, conflictNumber, DFGSize, SortedNodes, Nodes, conflictArray, T, selectionTimer, objectiveFunction, localAlgorithm
		, inRegisterConstraint, outRegisterConstraint, firstIteration);

	delete[] conflictArray;
	conflictArray = NULL;
}
//------------------------------------------------------------------------------------------------------
void findCI(ExprNode* SortedNodes, int DFGSize, TInOutNum T, int** TransitionMatrix, string basicFile, ExprNode* Nodes, 
			int clockConstraint, int inRegisterConstraint, int outRegisterConstraint, bool localAlgorithm, int objectiveFunction)
{//begin of findCI
	//inputs
	//1)SortedNodes:		an array that contains node's information that are in topological order
	//2)DFGSize:			size of input DFG
	//3)T:					input and output constraint
	//4)TransitionMatrix:	a transition matrix that shows the connection between nodes
	//5)basicFile:			stores the address of input file
	//6)Nodes:				an array that contains all node information
	//7)clockConstraint:	clock constraint
	//-----------------------------------------------------------------------------------------------
    bool* CIcand = new bool[DFGSize]; //a boolean array that shows the presence of a node in custom instruction
	memset(CIcand, false, DFGSize * sizeof(bool));//make array false
	//---------------------------------------------------------------------------------------------------------
	
	
	TInOutNum Tcut;//checks input and output of custom instruction with input and output constraint
	
	bool* LastCI = new bool[DFGSize];//a boolean array that shows the last custom instruction
	memset(LastCI, 0, DFGSize * sizeof(bool));
	LastCI[DFGSize - 1] = true;
	//---------------------------------------------------------------------------------------

	CIcand[0] = true;//put first node in custom instruction
	
	int CurrentLevel = 0;//checks last nodes that are changed in CIcand
	int numberOfCI = 0;//count the number of Custom Instruction
	int lenCI = 0;
	int i;

	//conditions for traverse binary tree
	bool DescendFlag = true;
	bool RightFlag = true;
	bool ContinueFlag = true;
	bool EqualFlag;
	bool Conditions;
	bool Convexity;
	int PermanentInputsNum = 0;
	//----------------------------------

	//calculate time of generation------------------------
	precision_timer generationTime;
	generationTime.start();

	CI* headerCI = new CI();
	int testCnt = 0;
	//----------------------------------------------------

	while(ContinueFlag)
	{//check continue Flag
		if(DescendFlag)
		{//check descend flag
			//cout << "its for test" << testCnt++ << endl;
			//assign default value to all variables
			Conditions = true;
			Tcut.inNum = Tcut.outNum = 0;
			PermanentInputsNum = 0;
			//-------------------------------------

			
			//function that find the permanent input number of custom instruction----------------------------
			Tcut =  findPermanentInputOutput(SortedNodes, CIcand, DFGSize, CurrentLevel, PermanentInputsNum);
			//-----------------------------------------------------------------------------------------------
			
			//checks output number and permanent number of custom instruction with constraints
			if(Tcut.outNum > T.outNum || PermanentInputsNum > T.inNum)
				Conditions = false;
			//--------------------------------------------------------------------------------

			//check if there is any forbiden node in CI----------------
			if(Conditions)
			{
				for(i = 0; i < DFGSize; i++)
					if(CIcand[i] == 1 && SortedNodes[i]._ForbiddenNode)
					{
						Conditions = false;
						break;
					}
			}
			//---------------------------------------------------------

			//check for convexity------------------------------------------
			if(Conditions)
			{
				Convexity = true;
				Convexity = CheckConvex(CIcand, TransitionMatrix, DFGSize);
				if(!Convexity)
					Conditions = false;
			}
			//-------------------------------------------------------------

			//checks if there are two or more nodes with different basic block number
			bool conflict = false;
			if(Conditions)
			{
				conflict = findConflict(CIcand, SortedNodes, DFGSize);
				if(conflict)
					Conditions = false;
			}
			//-----------------------------------------------------------------------

			//checks HWlatency with clock constraint---------------------------------
			if(Conditions)
			{
				double tempHWLatency;
				FindMeritValue(CIcand, SortedNodes, DFGSize, tempHWLatency);
				if(tempHWLatency > clockConstraint)
					Conditions = false;

			}
			//-----------------------------------------------------------------------

			if(Conditions)
			{
				//checks input port number of custom instruction with input constraint-----------------------------------
				if(Tcut.inNum <= T.inNum)
				{
					//function that stores the new custom instruction in global array
					updateCI(CIcand, SortedNodes, DFGSize, Tcut, lenCI);
					//---------------------------------------------------------------
					cout << "CI Number: " << numberOfCI++ << '\n'; //print the number of all custom instruction in output
					if (numberOfCI > 1000) break;
				}
				//-------------------------------------------------------------------------------------------------------

				//if the last node is equal to DFGSieze - 1 we should traverse tree in ascend mood
				if(CurrentLevel == DFGSize - 1)
				{
					DescendFlag = false;
					CIcand[CurrentLevel] = false;
					CurrentLevel--;
				}
				//--------------------------------------------------------------------------------

				//increment CurrentLevel by one and put next node in custom instruction
				else
				{
					CurrentLevel++;
					CIcand[CurrentLevel] = true;
				}
				//---------------------------------------------------------------------------------
			}

			//if any of the previous conditions are violated we should traverse tree in ascend mood
			else
			{
				DescendFlag = false;
				CIcand[CurrentLevel] = false;
				CurrentLevel--;
			}
			//--------------------------------------------------------------------------------------
		}

		//algorithm that traverse tree in other branches--------------------------------------------
		else
		{
			if(RightFlag)
				if(CurrentLevel != DFGSize - 2)
				{
					CurrentLevel = CurrentLevel + 2;
					CIcand[CurrentLevel] = true;
					DescendFlag = true;
				}
				else
				{
					if(CIcand[CurrentLevel])
						CIcand[CurrentLevel] = false;
					else
						RightFlag = false;
					CurrentLevel--;
				}
			else
			{
				if(CIcand[CurrentLevel])
				{
					CIcand[CurrentLevel] = false;
					RightFlag = true;
				}
				CurrentLevel--;
			}
		}
		//-----------------------------------------------------------------------------------------

		//checks this CIcand with the lastCIcand---------------------------------------------------
		EqualFlag = true;
		for(int i = 0; i < DFGSize; i++)
		{
			if(CIcand[i] != LastCI[i])
			{
				EqualFlag = false;
				break;
			}
		}
		if(EqualFlag)
			ContinueFlag = false;
		//-----------------------------------------------------------------------------------------
	}

	//assign default value to all variables
	Conditions = true;
	Tcut.inNum = Tcut.outNum = 0;
	PermanentInputsNum = 0;
	//--------------------------------------

	//function that find the permanent input number of custom instruction----------------------------
	Tcut =  findPermanentInputOutput(SortedNodes, CIcand, DFGSize, CurrentLevel, PermanentInputsNum);
	//-----------------------------------------------------------------------------------------------

	//checks output number and permanent number of custom instruction with constraints
	if(Tcut.outNum > T.outNum || PermanentInputsNum > T.inNum)
				Conditions = false;
	//--------------------------------------------------------------------------------

	//check if there is any forbiden node in CI----------------
	if(Conditions)
	{
		for(i = 0; i < DFGSize; i++)
			if(CIcand[i] == 1 && SortedNodes[i]._ForbiddenNode)
			{
				Conditions = false;
				break;
			}
	}
	//---------------------------------------------------------

	//check for convexity------------------------------------------
	if(Conditions)
	{
		Convexity = true;
		Convexity = CheckConvex(CIcand, TransitionMatrix, DFGSize);
		if(!Convexity)
			Conditions = false;
	}
	//-------------------------------------------------------------

	//checks if there are two or more nodes with different basic block number
	bool conflict = false;
	if(Conditions)
	{
		conflict = findConflict(CIcand, SortedNodes, DFGSize);
		if(conflict)
			Conditions = false;
	}
	//-----------------------------------------------------------------------

	if(Conditions)
	{
		if(Tcut.inNum <= T.inNum)
		{
			//function that stores the new custom instruction in global array
			updateCI(CIcand, SortedNodes, DFGSize, Tcut, lenCI);
			//---------------------------------------------------------------
			cout << "CI Number: " << numberOfCI++ << '\n';//print the number of all custom instruction in output
		}
	}
	//-----------------------------------------------------------------------------------------------------------

	generationTime.stop();//variable that store time information for proposed algorithm

	string strMkdir;
	char charObj[3];
	size_t mkdirFound;
	//---------mkdir--------
	mkdirFound = basicFile.find_last_of("\\");	
	strMkdir = basicFile;
	strMkdir.erase(mkdirFound + 1, strMkdir.size() - mkdirFound - 1);
		// --===
	//string mkdir=basicFile.find_last_of("\\");
	mkdirFound = strMkdir.find_last_of("\\");
	string direct =strMkdir;
	direct.erase(mkdirFound + 1, strMkdir.size() - mkdirFound - 1);
	direct[direct.length()-1]=NULL;
	string nameTask = direct;
	mkdirFound = direct.find_last_of("\\");
	direct[direct.length()-1]='\\';
	nameTask.erase(0,mkdirFound+1);
	nameTask[nameTask.length()-1]='.';
	string namePath=direct+nameTask+"txt";
	const char * c_namepath=namePath.c_str();
	CIall.open(c_namepath);
	direct ="D:\\education\\thesis_Msc\\allcode_v2\\BenchMark\\Reconfigurable characterstic\\profile\\";
	namePath =direct+"RFU_profile4size_"+nameTask+"txt";
	c_namepath=namePath.c_str();
	RFU_profile4size.open(c_namepath);
	RFU_profile4size << "usedInput_RFU\tusedOutput_RFU\tusedLevel_RFU\tclock_numbers\tspeedup\t<ROW>" << endl;
		//------------------------------
	if(localAlgorithm)
		strMkdir.insert(strMkdir.size(), "local-");
	else
		strMkdir.insert(strMkdir.size(), "global-");
	strMkdir.insert(strMkdir.size(), itoa(T.inNum, charObj, 10));
	strMkdir.insert(strMkdir.size(), itoa(T.outNum, charObj, 10));
	strMkdir.insert(strMkdir.size(), "-");
	strMkdir.insert(strMkdir.size(), "inR");
	strMkdir.insert(strMkdir.size(), itoa(inRegisterConstraint, charObj, 10));
	strMkdir.insert(strMkdir.size(), "-");
	strMkdir.insert(strMkdir.size(), "outR");
	strMkdir.insert(strMkdir.size(), itoa(outRegisterConstraint, charObj, 10));
	strMkdir.insert(strMkdir.size(), "-");
	strMkdir.insert(strMkdir.size(), "objfunc");
	strMkdir.insert(strMkdir.size(), itoa(objectiveFunction, charObj, 10));
	mkdir(strMkdir.c_str());
	//----------------------
	strMkdir.insert(strMkdir.size(), "\\tst");

	//------------------------------------------------------------------------------
	//combine all results
	/*ofstream fileCmbResults;
	string strCmbResults(strMkdir);
	size_t sizeCmbFound, sizeCmbEnd;
	string strFuncName, strAlgName;
	char bufCmbResult[200];
	char tempInt[10];
	sizeCmbFound =  strCmbResults.find_last_of("\\");
	strCmbResults.erase(sizeCmbFound , strCmbResults.size() - sizeCmbFound);
		
	sizeCmbFound =  strCmbResults.find_last_of("\\");
	strCmbResults.erase(sizeCmbFound, strCmbResults.size() - sizeCmbFound);

	sizeCmbFound =  strCmbResults.find_last_of("\\");
	strFuncName.append(strCmbResults, sizeCmbFound + 1, strCmbResults.size()); 
	strCmbResults.erase(sizeCmbFound, strCmbResults.size() - sizeCmbFound);

	sizeCmbFound =  strCmbResults.find_last_of("\\");
	strCmbResults.erase(sizeCmbFound, strCmbResults.size() - sizeCmbFound);
	
	sizeCmbFound =  strCmbResults.find_last_of("\\");
	strAlgName.append(strCmbResults, sizeCmbFound + 1, strCmbResults.size());

	strCmbResults.insert(strCmbResults.size(), "\\");
	if(localAlgorithm == true)
		strCmbResults.insert(strCmbResults.size(), "local");
	else
		strCmbResults.insert(strCmbResults.size(), "global");
	strCmbResults.insert(strCmbResults.size(), "-");
	strCmbResults.insert(strCmbResults.size(), itoa(T.inNum, tempInt, 10));
	strCmbResults.insert(strCmbResults.size(), itoa(T.outNum, tempInt, 10));
	strCmbResults.insert(strCmbResults.size(), "-");
	strCmbResults.insert(strCmbResults.size(), "inR");
	strCmbResults.insert(strCmbResults.size(), itoa(inRegisterConstraint, tempInt, 10));
	strCmbResults.insert(strCmbResults.size(), "-");
	strCmbResults.insert(strCmbResults.size(), "outR");
	strCmbResults.insert(strCmbResults.size(), itoa(outRegisterConstraint, tempInt, 10));
	strCmbResults.insert(strCmbResults.size(), "-");
	strCmbResults.insert(strCmbResults.size(), "objfunc");
	strCmbResults.insert(strCmbResults.size(), itoa(objectiveFunction, tempInt, 10));
	mkdir(strCmbResults.c_str());*/
	//------------------------------------------------------------------------------
	//operations to write generation time of tree in a file-------------------------
	size_t genFound, genBuff, genBuffFinal;
	ofstream generationFile;
	basicFile = strMkdir;
	string strGenerationTime(basicFile);
	char buffGen[200];
	genFound = basicFile.find_last_of("\\");
	strGenerationTime.erase(genFound + 1, strGenerationTime.size() - genFound - 1);
	strGenerationTime.insert(strGenerationTime.size(), "generationTime.txt");
	genBuff = strGenerationTime.size();
	genBuffFinal = strGenerationTime.copy(buffGen, genBuff, 0);
	buffGen[genBuffFinal] = '\0';
	
	generationFile.open(buffGen);
	generationFile <<"Tree Generation Time(microSec): " << generationTime << endl; //tree generation
	generationFile <<"Number of all CIs: " << numberOfCI;//number of custom instruction that were generated
	generationFile.close();
	//-------------------------------------------------------------------------------

	bool firstIteration = true;

	while(T.inNum != 2)
	{
		T.outNum = T.inNum;
		while(T.outNum != 0)
		{
			if(firstIteration)
			{
				makeConflictGraph(lenCI, basicFile, DFGSize, SortedNodes, Nodes, T, inRegisterConstraint, 
								outRegisterConstraint, localAlgorithm, objectiveFunction, clockConstraint, true);
				firstIteration = false;
			}
			else
				makeConflictGraph(lenCI, basicFile, DFGSize, SortedNodes, Nodes, T, inRegisterConstraint, 
								outRegisterConstraint, localAlgorithm, objectiveFunction, clockConstraint, false);

			T.outNum--;
		}
		T.inNum--;
	}

}

//------------------------------------------------------------------------------------------------
//reads tree structure from a file and return the basic block number and size of DFG
void readDFGInputFile(string FileName, int *DFGSize, ExprNode* inputNodes, int offset, int basicBlock, int iter)
{
	FILE *F;
    Operations* allOperations[20];
	readDelayArea("D:\\education\\thesis_Msc\\allcode_v2\\CI_reconfig\\CI_extract_reconfig\\areadelay.txt", allOperations);

	char line[1000];
	int steps = 0;
	int nodeNum =0;
	int index;
	int NodeID;
	int OutputTemp[1000];
	int OutputIndexVector;
	int InputTemp[1000];
	int inputIndexVector;
	int operationIndex = -1;

	
	string temp;

	F = fopen(/*"H:\\thesis_implementation\\Extrac_CIs\\temporary\\BB0.txt"*/FileName.c_str(), "r");
	while(!feof(F))
	{
		fgets(line, 1000, F);
		index = 0;
		if (line[0] == '#')
			if((line[1] == 'e') &&(line[2] == 'n')&&(line[3] == 'd'))
				break;
			else
				continue;
		if (steps == 0) //read the number of Nodes
		{
			//edired by rahim----------
			/*nodeNum = atoi(line);
			(*DFGSize) = (*DFGSize) + nodeNum;*/
			steps++;
			//nodeNum = 0;
		}
		else //Nodes Describtion
		{
			nodeNum ++;
			//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
			temp = "";	
			while(line[index] != ' ') //extract node id
				temp += line[index++];
			NodeID = atoi(temp.c_str());
			inputNodes[(NodeID - 1) + offset]._Id = NodeID/*-1)===*/ + offset;
			inputNodes[(NodeID - 1) + offset].basicNum = basicBlock;
			inputNodes[(NodeID - 1) + offset].iteration = iter;
			inputNodes[(NodeID - 1) + offset].isTraced = false;
			inputNodes[(NodeID - 1) + offset].lastID = NodeID + offset;
			inputNodes[(NodeID - 1) + offset].flagLevel = false;

			while(line[index++] == ' ');
			index--;
			temp = "";//new string;	
			while(line[index] != ' ')//extract operation
				temp += line[index++];
		    inputNodes[(NodeID - 1) + offset]._Op = temp;
			//-------Check Forbidden Nodes-----------------
			if((strcmp(temp.c_str(), "LD") == 0)||(strcmp(temp.c_str(), "ST") == 0)||(strcmp(temp.c_str(), "CONS") == 0)||(strcmp(temp.c_str(), "IN") == 0))
				inputNodes[(NodeID - 1) + offset]._ForbiddenNode = true;
			else
				inputNodes[(NodeID - 1) + offset]._ForbiddenNode = false;
			//---------------------------------------------
			while(line[index++] == ' '); 
			index--;
			temp = "";	
			while(line[index] != ' ')//extract SW time
				temp += line[index++];
			inputNodes[(NodeID - 1) + offset]._SwTime = 1;//atof(temp.c_str());

			while(line[index++] == ' ');
			index--;
			temp = "";	
			while(line[index] != ' ')//extract HW time
				temp += line[index++];
			operationIndex = -1;
			//-----------------------------------register file as temporary : start
			if(inputNodes[(NodeID - 1) + offset]._Op.compare("IN") == 0)
			{
				inputNodes[(NodeID - 1) + offset].regFile= ((NodeID - 1) + offset)%32;
			}
			//------------------------------------register file as temporary : end
			if(inputNodes[(NodeID - 1) + offset]._Op.compare("SHR") == 0)
			{
				operationIndex = 11;
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("SHL") == 0)
			{
				operationIndex = 12;
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("ADD") == 0)
			{
				operationIndex = 0;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("REL") == 0)
			{
				operationIndex = 13;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("MIN") == 0)
			{
				operationIndex = 14;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("OR") == 0)
			{
				operationIndex = 5;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("ORC") == 0)
			{
				operationIndex = 6;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("AND") == 0)
			{
				operationIndex = 2;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("SUB") == 0)
			{
				operationIndex = 1;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("EQT") == 0)
			{
				operationIndex = 7;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("SLCT") == 0)
			{
				operationIndex = 15;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("XOR") == 0)
			{
				operationIndex = 16 ;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("SHADD") == 0)
			{
				operationIndex = 10;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("NAND") == 0)
			{
				operationIndex = 3;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("NOR") == 0)
			{
				operationIndex = 4;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("XNOR") == 0)
			{
				operationIndex = 17;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("SZXT") == 0)
			{
				operationIndex = 18;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("MUL") == 0)
			{
				operationIndex = 8;			
			}
			else if (inputNodes[(NodeID - 1) + offset]._Op.compare("IMUL") == 0)
			{
				operationIndex = 9;			
			}
			else
			{
				cout << "Unknown node: " << inputNodes[(NodeID - 1) + offset]._Op << '\n';
			}
			if(operationIndex != -1)
			{
				inputNodes[(NodeID - 1) + offset]._HwTime = allOperations[operationIndex]->_Delay;
				inputNodes[(NodeID - 1) + offset]._Area = allOperations[operationIndex]->_Area;
				inputNodes[(NodeID - 1) + offset]._DynamicPower = allOperations[operationIndex]->_DynamicPower;
				inputNodes[(NodeID - 1) + offset]._immediateDynamicPower = allOperations[operationIndex]->_DynamicPowerImmidediate;
			}
			else
			{
				inputNodes[(NodeID - 1) + offset]._HwTime = allOperations[2]->_Delay;
				inputNodes[(NodeID - 1) + offset]._Area = allOperations[2]->_Area;
				inputNodes[(NodeID - 1) + offset]._DynamicPower = allOperations[2]->_DynamicPower;
				inputNodes[(NodeID - 1) + offset]._immediateDynamicPower =allOperations[2]->_DynamicPowerImmidediate;
			}
			while(line[index++] == ' ');
			index--;
			temp = "";	
			while(line[index] != ' ')
				temp += line[index++];
			inputNodes[(NodeID - 1) + offset]._UnconnectedInputs = atoi(temp.c_str());

			inputNodes[(NodeID - 1) + offset]._UnconnectedOutputs = 0;
			//----------Find Neighbors-------------------------
			while(line[index++] == ' ');
			if(line[index - 1] != '<')
				cout<<" Parsing Error ....\n";
			inputNodes[(NodeID - 1) + offset]._NumberOfOutputs = 0;
			OutputIndexVector = 0;

			while(line[index] != '>')
			{
				while(line[index++] == ' ');
				if(line[index - 1] == ',')
					while(line[index++] == ' ');
				if(line[index - 1] == '>')
					break;
				index--;
				temp = "";
				while(line[index] != ' ')
					temp += line[index++];
				OutputTemp[OutputIndexVector] = atoi(temp.c_str()) - 1;
				OutputIndexVector++;
			}
			if(OutputIndexVector != 0) //number of outputs and pointer
			{
				inputNodes[(NodeID - 1) + offset]._NumberOfOutputs = OutputIndexVector;
				inputNodes[(NodeID - 1) + offset]._NumberOfRealOutputs = OutputIndexVector;
				for(int i=0;i<OutputIndexVector; i++)//push all output in output
				{
					inputNodes[(NodeID - 1) + offset]._Output.push_back(OutputTemp[i] + offset);
					inputNodes[(NodeID - 1) + offset]._ReOutput.push_back(OutputTemp[i] + offset);
				}
			}
            
			inputNodes[(NodeID - 1) + offset].traceFlag = false;
		}

	}
	//-------------Find the inputs of Each Node-----------
	//---------------- ===
	(*DFGSize) = (*DFGSize) + nodeNum;
	//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
	for(int i =0; i<nodeNum ;i++)
	{
		inputIndexVector = 0;
		inputNodes[i + offset]._NumParents = 0;
		inputNodes[i + offset].config_level=-1;
		inputNodes[i + offset].config_row=-1;
		for(int j=0; j<nodeNum; j++)
		{
			if (i == j) 
				continue;
			for(int k=0; k< inputNodes[j+offset]._NumberOfOutputs; k++)
			{
				if (inputNodes[j+offset]._Output[k] == i+offset)
				{
					InputTemp[inputIndexVector] = j;
					inputIndexVector++;
					break;
				}//if
			}//for k
		}//for j
		if(inputIndexVector != 0)
		{
			inputNodes[i+offset]._NumParents = inputIndexVector;
			inputNodes[i+offset]._UnconnectedInputs -= inputIndexVector;
			for(int k =0;k<inputIndexVector; k++)
			{
				inputNodes[i+offset]._Parents.push_back(InputTemp[k] + offset);
				inputNodes[i+offset]._ReParents.push_back(InputTemp[k] + offset);
			}

		}
	}
}