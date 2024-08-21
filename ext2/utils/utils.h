


#define AbortIfBad(good, msg) \
{\
if(!good)\
{\
	fprintf(stderr,"%s Creation:", msg);\
	exit(0);\
}\
}

#define ExitIfBad(good) \
{\
if(!good)\
{\
	exit(0);\
}\
}

#define ReturnIfBad(good, err, msg) \
{\
if(!good)\
{\
	fprintf(stderr,"%s Creation:", msg);\
	return err;\
}\
}

#define ReturnIfBadUpdatPerror(good, err, msg) \
{\
if(!good)\
{\
	perror(msg);\
	return err;\
}\
}




