static const int NPAD		= 7;
static const int ARRAY_SIZE	= 512;

struct MyStruct {
	struct MyStruct *n;
	long int pad[NPAD];
};