#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;
//return number of operation in file and fill an array of operation
int readDelayArea(char* fileName, Operations* Op[])
{
	FILE * fileAreaDelay;
	double area;
	double delay;
	double DynamicPower;
	double DynamicPowerImmediate;

	char opStr [10];

	fileAreaDelay = fopen(fileName,"r");
	int operationIndex = 0 ;
	while(fscanf(fileAreaDelay ,"%s %20Lf %20Lf %20Lf %20Lf", opStr, &area, &delay, &DynamicPower, &DynamicPowerImmediate) != EOF)
	{
		Op[operationIndex] = new Operations();
		strcpy(Op[operationIndex]->_Op, opStr);
		Op[operationIndex]->_Delay = delay;
		Op[operationIndex]->_Area = area;
		Op[operationIndex]->_DynamicPower = DynamicPower;
		Op[operationIndex]->_DynamicPowerImmidediate = DynamicPowerImmediate;
		operationIndex++;
	}
	
	/*for(int i = 0; i < operationIndex; i++)
		cout <<"Operation: " << Op[i]->_Op << " Delay: " << Op[i]->_Delay << " Area: " << Op[i]->_Area << endl;
		*/
	fclose(fileAreaDelay);
	return operationIndex;
}

string char2String(char* in)
{
	stringstream ss;
	string Result;
	ss << in;
	ss >> Result;
	return Result;
}

int string2int(string in)
{
	int result;
	char iterationInt[20];
    strcpy(iterationInt, in.c_str());
	result = atoi(iterationInt);
	return result;
}