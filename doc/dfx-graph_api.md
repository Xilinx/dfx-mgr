# DFX-MGR-Graph API

## Graph_Create

**Arguments:** 
	
	* const char* name

**Returns:** GRAPH_HANDLE

This function initialise a graph object and returns its handle


## Graph_CreateWithPriority

**Arguments:**

	* const char* name
	* int priority

**Returns:** GRAPH_HANDLE

This function initialise a graph object with a assigned priority (range 0-3) and returns its handle

## Graph_Distroy

**Arguments:**
	
	* GRAPH_HANDLE gHandle
	
**Returns:** 0 on success and -1 on error

This function destroys graph object


## Graph_getInfo

**Arguments:**
	
	* GRAPH_HANDLE gHandle
	
**Returns:** char* Graph Information in printable format

This function returns graph debug info in a string.


## Graph_getInfo

**Arguments:**
	
	* GRAPH_HANDLE gHandle
	
**Returns:** char* Graph Information in printable format

This function returns graph debug info in a string.

## Graph_addAccelByName

**Arguments:**

	* GRAPH_HANDLE Parent graph handeler 
	* const char* name of the accelerator
	
**Returns:** ACCEL_HANDLE

This function adds an accelerator to the graph. Accelerator name needs to be provided as an argument.

## Graph_addFallbackAccelByName

**Arguments:**

	* GRAPH_HANDLE Parent graph handeler 
	* const char* name of the accelerator
	
**Returns:** ACCEL_HANDLE

This function adds an accelerator to the graph. The accelerator forcefully defaults to software fallback. Accelerator name needs to be provided as an argument.


## Graph_addInputNode

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler 
	* const char*: name
	* int: size of buffer
	
**Returns:** ACCEL_HANDLE

This function adds an Input node buffer to the graph. This node acts as a input data buffer to the application.


## Graph_addOutputNode

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler 
	* const char*: name
	* int: size of buffer
	
**Returns:** ACCEL_HANDLE

This function adds an Output node buffer to the graph. This node acts as a output data buffer to the application.

## Graph_addBufferByName

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler 
	* const char*: name
	* int: size of buffer
	* int: Buffer type 
		* 0: DDR based buffer
		* 1: Streaming interconnect based logical buffer
	
**Returns:** ACCEL_HANDLE

This function adds an Interconnect buffer that can be used to connect two accelerators or an IO node to an accelerator.

## Graph_connectInputBuffer

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler 
	* ACCEL_HANDLE: Accelerator Handle
	* BUFFER_HANDLE: Buffer Handle
	* int offset
	* int transactionSize
	* int transactionIndex
	* int channel
	
**Returns:** LINK_HANDLE

This function connects a buffer to an accelerator on the input end.

## Graph_connectOutputBuffer

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler 
	* ACCEL_HANDLE: Accelerator Handle
	* BUFFER_HANDLE: Buffer Handle
	* int offset
	* int transactionSize
	* int transactionIndex
	* int channel
	
**Returns:** LINK_HANDLE

This function connects a buffer to an accelerator on the output end.


## Graph_delBuffer

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler 
	* BUFFER_HANDLE: Buffer Handle
	
**Returns:** 0 on success and -1 on error

This function removes a buffer and its all connections from a graph.


## Graph_delLink

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler 
	* LINK_HANDLE: Buffer Handle
	
**Returns:** 0 on success and -1 on error

This function removes a connection from a graph.

## Graph_countAccel

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler
	
**Returns:** Number of accelerators in the graph

This function returns number of accelerators from a graph.

## Graph_countBuffer

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler
	
**Returns:** Number of connecting buffers in the graph

This function returns number of connecting buffers from a graph.

## Graph_countLink

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler
	
**Returns:** Number of links in the graph

This function returns number of links from a graph.

## Graph_jsonAccels

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler
	
**Returns:** Json string 

This function returns Json string containing list of accelerators and their attributes.


## Graph_jsonBuffers

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler
	
**Returns:** Json string 

This function returns Json string containing list of buffers and their attributes.


## Graph_jsonLinks

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler
	
**Returns:** Json string 

This function returns Json string containing list of Links and their attributes.



## Graph_toJson

**Arguments:**

	* GRAPH_HANDLE: Parent graph handeler
	
**Returns:** Json string 

This function returns Json string containing list of accelerators, buffers and Links with their attributes.

