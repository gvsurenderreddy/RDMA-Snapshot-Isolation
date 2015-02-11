/*
 *	item.hpp
 *
 *	Created on: 22.Jan.2015
 *	Author: erfanz
 */

#include <iostream>
using namespace std;

class Item {
public:
	int I_ID;
	char I_TITLE[60];
	int I_A_ID;
	char I_PUB_DATE[11];
	char I_PUBLISHER[60];
	char I_SUBJECT[60];
	char I_DESC[500];
	int I_RELATED1;
	int I_RELATED2;
	int I_RELATED3;
	int I_RELATED4;
	int I_RELATED5;
	char I_THUMBNAIL[40];
	char I_IMAGE[40];
	double I_SRP;
	double I_COST;
	char I_AVAIL[10];
	int I_STOCK;
	char I_ISBN[13];
	int I_PAGE;
	char I_BACKING[15];
	char I_DIMENSION[25];	
	
	
	friend std::ostream& operator<<(std::ostream&, const Item&);	// overriding << operation for printing the object
};

std::ostream& operator<<(std::ostream &strm, const Item &item) {
	return strm << "Item("
		<< item.I_ID << ", "
			<< item.I_TITLE << ", "
				<< item.I_A_ID << ", "
					<< item.I_PUB_DATE << ", "
						<< item.I_PUBLISHER << ", "
							<< item.I_SUBJECT << ", "
								<< item.I_DESC << ", "
									<< item.I_RELATED1 << ", "
										<< item.I_RELATED2 << ", "
											<< item.I_RELATED3 << ", "
												<< item.I_RELATED4 << ", "
													<< item.I_RELATED5 << ", "
														<< item.I_THUMBNAIL << ", "
															<< item.I_IMAGE << ", "
																<< item.I_SRP << ", "
																	<< item.I_COST << ", "
																		<< item.I_AVAIL << ", "
																			<< item.I_STOCK << ", "
																				<< item.I_ISBN << ", "
																					<< item.I_PAGE << ", "
																						<< item.I_BACKING << ", "
																							<< item.I_DIMENSION << ", "
																								<<  ")" << endl;
}