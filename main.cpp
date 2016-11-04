#include <stdio.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <complex>
#include "class.h"
#include "basicBlock.h"
using namespace std;
//----------------------------
int main(int argc, char* argv[])
{

	vector<string> args;
	
	int i, j;
	//default value for inputs---------------
	int		inRegisterConstraint	= 2;
	int		outRegisterConstarint	= 1;
	int		inputConstraint			= 4;
	int		outputConstraint		= 4;
	int		objectiveFunction		= 1;
	int		clockConstraint			= 1;
	bool	localAlgorithm			= false;
	bool	globalAlgorithm			= true;
	//end------------------------------------

	//put inputs in a vector------------
	for(i = 1; i < argc; i++)
		args.push_back(argv[i]);
	//end-------------------------------

	//check that not both of local and global--------------------
	bool checkLocal		=	false;
	bool checkGlobal	=	false;
	for(i = 1; i < argc - 1; i++)
	{
		if(!args[i].compare("-local"))
			checkLocal	=	true;
		if(!args[i].compare("-global"))
			checkGlobal	=	true;

	}
	if(checkGlobal && checkLocal)
		return 0;
	//------------------------------------------------------------

	//check argc for valid switch--------------------
	for(i = 1; i < argc - 1; i++)
	{
		if(!args[i].compare("-incons"))
			inputConstraint = atoi(args[i+1].c_str());
		else if(!args[i].compare("-outcons"))
			outputConstraint = atoi(args[i+1].c_str());
		else if(!args[i].compare("-objfunc"))
			objectiveFunction = atoi(args[i+1].c_str());
		else if(!args[i].compare("-rin"))
			inRegisterConstraint = atoi(args[i+1].c_str());
		else if(!args[i].compare("-rout"))
			outRegisterConstarint = atoi(args[i+1].c_str());
		else if(!args[i].compare("-clock"))
			clockConstraint = atoi(args[i+1].c_str());
		else if(!args[i].compare("-local"))
		{
			globalAlgorithm =	false;
			localAlgorithm	=	true;
		}
		else if(!args[i].compare("-global"))
		{
			globalAlgorithm =	true;
			localAlgorithm	=	false;
		}
	}
	//end-------------------------------------------
	
	
	int intOffset = 0; //incremental offset

	ifstream basicF; //all BB files should be here
	string mainFile; //it has all BB's Path

	string strFileName;


	ExprNode* Nodes = new ExprNode[1000];//contains all nodes
	ExprNode* SortedNodes = new ExprNode[1000];//contains all nodes in topological order
	
	cout << "Test File is --> " << argv[1] << "\n";
	basicF.open(argv[1]);//
	
	int DFGSize = 0; //size of DFG
	
	string strNumIter; // string that stores number of iteration
	int intIterationNum; //integer for iteration number
	int intBB = 0; //integer for each basic block

	//make a tree based on input file
	while(!basicF.eof())
	{//while for fill Nodes
		cout << "<----------------------> " << "\n";
		
		getline(basicF, strFileName);//get basic block file name				
		getline(basicF, strNumIter);
		if(strFileName == "")
			break;
		intIterationNum = string2int(strNumIter); //convert iteration string to integer		
		cout << strFileName <<endl;
		readDFGInputFile(strFileName, &DFGSize, Nodes, intOffset, intBB, intIterationNum); //read DFG
		intBB++; //increment basic block number by one
		intOffset = DFGSize; //offset		
				
		cout << "DFG size is = " << DFGSize; //print each DFG size
	}
	basicF.close();	
	//------------------------------------------------------------------------------------

	fillOrder(Nodes, DFGSize);//functions that give an order to each node from output
	
	
	intOffset = 0;
	
	intBB = 0;
	int intTempDFG = 0;

	//make a tree for topological sort------------------------------------------------------------
	ifstream sortedFile; //the same file for sorted nodes
	sortedFile.open(argv[1], ios::beg);
	while(! sortedFile.eof())
	{		
		getline(sortedFile, strFileName);
		if(strFileName == "")
			break;
		getline(sortedFile, strNumIter);
		intIterationNum = string2int(strNumIter);				
		readDFGInputFile(strFileName, &intTempDFG, SortedNodes, intOffset, intBB, intIterationNum);
		intBB++;
		intOffset = intTempDFG;
		cout << "DFG size is = " << intTempDFG;
	}
	sortedFile.close();
	//---------------------------------------------------------------------------------------------
    
	//function that sorts input based on their order
	topologicalSortDFG(SortedNodes, Nodes, DFGSize);
	//----------------------------------------------

	//function that checks input of basic operator for constant and put HWLatency for SHL and SHR to zero
	checkConstant(SortedNodes, DFGSize);
	//---------------------------------------------------------
	
	//make transition matrix-----------------------------------
	int** TransitionMatrix = new int*[DFGSize];
	for(i = 0; i < DFGSize; i++)
		TransitionMatrix[i] = new int[DFGSize];
	
	for(i = 0; i < DFGSize; i++)
		for(j = 0; j < DFGSize; j++)
			TransitionMatrix[i][j] = 0;

	for(i = 0; i < DFGSize; i++)
		for(j = 0; j < SortedNodes[i]._NumberOfOutputs; j++)
		{
			if(SortedNodes[i]._Output[j] < 0)
				
			{
				SortedNodes[i]._NumberOfOutputs--;
				SortedNodes[i]._Output[j]=0;
				//break;
			}
			TransitionMatrix[i][SortedNodes[i]._Output[j]] = 1;
		}
	//---------------------------------------------------------

	TInOutNum T;

	string basicFile( argv[1] ); //new string to pass to findCI
	
	T.inNum = inputConstraint;
	T.outNum = outputConstraint;
	
	//just for clarity-----------------------------------
	cout << "Input constraint is: " << T.inNum << endl;
	cout << "Output constraint is: " << T.outNum << endl;
	//---------------------------------------------------

	//this function finds all Custom Instructions that are valid(e.g. Convex, No Forbidden Node, No Violation in Input Constraint 
	//and Output Constraitn and Clock Constraint)	
	findCI(SortedNodes, DFGSize, T, TransitionMatrix, basicFile, Nodes, clockConstraint, inRegisterConstraint, outRegisterConstarint, localAlgorithm, objectiveFunction);	
	return 0;
}