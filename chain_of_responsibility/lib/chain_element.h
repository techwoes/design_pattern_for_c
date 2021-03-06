/**
 * @file chain_element.h
 * @brief This is API for element of chain
**/
#ifndef CHAIN_ELEMENT_H_
#define CHAIN_ELEMENT_H_
#include "chain_of_responsibility.h"

/* @{ */
#define COR_SUCCESS (0)
#define COR_FAILED (-1)
/* @} */

/*! @struct chain_element
 * @brief ChainElement class instance definition, detail is in chain_element.c
*/
struct chain_element_t;/*! extend DPUtilListData defined in design_pattern_util/include/dp_list.h */
typedef struct chain_element_t chain_element_t, *ChainElement;

struct chain_element_data_t {
	chain_func func;
	void *ctx;
};
typedef struct chain_element_data_t chain_element_data_t;

struct chain_element_req_t {
	chain_element_data_t element_data;
	ChainElementPart result_element_part;
};
typedef struct chain_element_req_t chain_element_req_t;

typedef struct chain_element_data_t chain_element_data_t;
ChainElement chain_element_new(int is_threadsafe);
int chain_element_add_function(ChainElement this, chain_element_req_t * elemnt_data);
void chain_element_remove_function(ChainElement this, chain_func func);
void chain_element_remove_element_part(ChainElement this, ChainElementPart element);
void chain_element_call(ChainElement this, void *arg);
void chain_element_delete(ChainElement this);
#endif
