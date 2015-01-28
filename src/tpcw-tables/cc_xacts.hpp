#include <iostream>
using namespace std;

class CCXacts {
public:
	int CX_O_ID;
	char CX_TYPE[10];
	char CX_NUM[20];
	char CX_NAME[30];
	char CX_EXPIRY[10];
	char CX_AUTH_ID[15];
	double CX_XACT_AMT;
	char CX_XACT_DATE[10];
	int CX_CO_ID;
};