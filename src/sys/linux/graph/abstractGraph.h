typedef struct AbstractBuffNode AbstractBuffNode_t;
typedef struct AbstractAccelNode AbstractAccelNode_t;
typedef struct AbstractLink AbstractLink_t;
typedef struct AbstractGraph AbstractGraph_t;
typedef struct Element Element_t;
typedef struct graphSocket graphSocket_t;

struct Element{
	void* node;
        struct Element *head;
        struct Element *tail;
};

struct AbstractAccelNode{
        uint8_t type;
        char name[256];
        uint32_t size;
        uint32_t id;
        //int SchedulerBypassFlag;
};

struct AbstractBuffNode{
        uint8_t type;
        char name[256];
        uint32_t size;
        uint32_t id;
};

struct AbstractLink{
        AbstractAccelNode_t *accelNode;// Reference to connected accelerator
        AbstractBuffNode_t *buffNode;// Reference to connected buffer
        uint8_t type;
        uint8_t transactionIndex;
        uint32_t transactionSize;
        uint32_t offset;
        uint8_t channel;
        uint32_t id;
};

struct AbstractGraph{
        uint32_t id;
        uint8_t type;
	graphSocket_t *gs;
        Element_t *accelNodeHead;
        Element_t *buffNodeHead;
        Element_t *linkHead;
        uint32_t accelNodeID;
        uint32_t buffNodeID;
        uint32_t linkID;
};



//extern AcapGraph_t* graphInit();
//extern int graphFinalise(AcapGraph_t *acapGraph);
extern AbstractGraph_t* graphInit(); //uint8_t schedulerBypassFlag);
extern int graphFinalise(AbstractGraph_t *graph);

extern AbstractAccelNode_t* addInputNode(AbstractGraph_t *graph, int size);
extern AbstractAccelNode_t* addOutputNode(AbstractGraph_t *graph, int size);
extern AbstractAccelNode_t* addAcceleratorNode(AbstractGraph_t *graph, char *name);
extern AbstractBuffNode_t* addBuffer(AbstractGraph_t *graph, int size, int type);
extern AbstractLink_t *addOutBuffer(AbstractGraph_t *graph, AbstractAccelNode_t *accelNode, AbstractBuffNode_t *buffNode,
			    uint32_t offset, uint32_t transactionSize, uint8_t transactionIndex, uint8_t channel);

extern AbstractLink_t *addInBuffer(AbstractGraph_t *graph, AbstractAccelNode_t *accelNode, AbstractBuffNode_t *buffNode,
			    uint32_t offset, uint32_t transactionSize, uint8_t transactionIndex, uint8_t channel);

extern int abstractGraph2Json(AbstractGraph_t *graph, char* json);
extern int abstractGraphConfig(AbstractGraph_t *graph);
extern int abstractGraphServerConfig(Element_t **GraphList, char* json, int len);
extern int abstractGraphFinalise(AbstractGraph_t *graph);
