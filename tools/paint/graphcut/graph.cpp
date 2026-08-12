/* graph.cpp */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits>
#include <new>
#include <stdexcept>
#include "graph.h"

namespace
{
bool checked_multiply(std::size_t lhs, std::size_t rhs, std::size_t &result)
{
	if (lhs != 0 && rhs > std::numeric_limits<std::size_t>::max()/lhs)
		return false;
	result = lhs*rhs;
	return true;
}

bool checked_add(std::size_t lhs, std::size_t rhs, std::size_t &result)
{
	if (rhs > std::numeric_limits<std::size_t>::max()-lhs)
		return false;
	result = lhs+rhs;
	return true;
}

void report_error(void (*error_function)(char *), const char *message)
{
	if (error_function)
		(*error_function)(const_cast<char *>(message));
}
}


template <typename captype, typename tcaptype, typename flowtype>
	bool Graph<captype,tcaptype,flowtype>::estimate_memory(int node_num_max,
							int edge_num_max,
							std::size_t &bytes)
{
	bytes = 0;
	if (node_num_max < 0 || edge_num_max < 0)
		return false;

	if (node_num_max < 16) node_num_max = 16;
	if (edge_num_max < 16) edge_num_max = 16;
	if (edge_num_max > std::numeric_limits<int>::max()/2)
		return false;

	std::size_t node_bytes = 0;
	std::size_t arc_bytes = 0;
	if (!checked_multiply(static_cast<std::size_t>(node_num_max),
			      sizeof(node), node_bytes) ||
	    !checked_multiply(static_cast<std::size_t>(edge_num_max)*2,
			      sizeof(arc), arc_bytes))
		return false;

	return checked_add(node_bytes, arc_bytes, bytes);
}


template <typename captype, typename tcaptype, typename flowtype> 
	Graph<captype, tcaptype, flowtype>::Graph(int node_num_max, int edge_num_max, void (*err_function)(char *))
	: nodes(NULL),
	  node_last(NULL),
	  node_max(NULL),
	  arcs(NULL),
	  arc_last(NULL),
	  arc_max(NULL),
	  node_num(0),
	  nodeptr_block(NULL),
	  error_function(err_function),
	  flow(0),
	  maxflow_iteration(0),
	  changed_list(NULL)
{
	std::size_t allocation_bytes = 0;
	if (!estimate_memory(node_num_max, edge_num_max, allocation_bytes))
	{
		report_error(error_function, "Graph capacity is too large!");
		throw std::length_error("Graph capacity is too large");
	}
	(void)allocation_bytes;

	if (node_num_max < 16) node_num_max = 16;
	if (edge_num_max < 16) edge_num_max = 16;

	std::size_t node_bytes = 0;
	std::size_t arc_bytes = 0;
	checked_multiply(static_cast<std::size_t>(node_num_max), sizeof(node), node_bytes);
	checked_multiply(static_cast<std::size_t>(edge_num_max)*2, sizeof(arc), arc_bytes);

	nodes = static_cast<node *>(malloc(node_bytes));
	if (!nodes)
	{
		report_error(error_function, "Not enough memory!");
		throw std::bad_alloc();
	}

	arcs = static_cast<arc *>(malloc(arc_bytes));
	if (!arcs)
	{
		free(nodes);
		nodes = NULL;
		report_error(error_function, "Not enough memory!");
		throw std::bad_alloc();
	}

	node_last = nodes;
	node_max = nodes + node_num_max;
	arc_last = arcs;
	arc_max = arcs + 2*edge_num_max;

}

template <typename captype, typename tcaptype, typename flowtype> 
	Graph<captype,tcaptype,flowtype>::~Graph()
{
	if (nodeptr_block) 
	{ 
		delete nodeptr_block; 
		nodeptr_block = NULL; 
	}
	free(nodes);
	free(arcs);
}

template <typename captype, typename tcaptype, typename flowtype> 
	void Graph<captype,tcaptype,flowtype>::reset()
{
	node_last = nodes;
	arc_last = arcs;
	node_num = 0;

	if (nodeptr_block) 
	{ 
		delete nodeptr_block; 
		nodeptr_block = NULL; 
	}

	maxflow_iteration = 0;
	flow = 0;
}

template <typename captype, typename tcaptype, typename flowtype> 
	void Graph<captype,tcaptype,flowtype>::reallocate_nodes(int num)
{
	if (num <= 0 || node_num > std::numeric_limits<int>::max()-num)
	{
		report_error(error_function, "Graph node capacity is too large!");
		throw std::length_error("Graph node capacity is too large");
	}

	const int old_node_num_max = static_cast<int>(node_max - nodes);
	const int required_node_num = node_num + num;
	std::size_t node_num_max = static_cast<std::size_t>(old_node_num_max);
	node* nodes_old = nodes;

	node_num_max += node_num_max / 2;
	if (node_num_max < static_cast<std::size_t>(required_node_num))
		node_num_max = static_cast<std::size_t>(required_node_num);
	if (node_num_max > static_cast<std::size_t>(std::numeric_limits<int>::max()))
	{
		report_error(error_function, "Graph node capacity is too large!");
		throw std::length_error("Graph node capacity is too large");
	}

	std::size_t node_bytes = 0;
	if (!checked_multiply(node_num_max, sizeof(node), node_bytes))
	{
		report_error(error_function, "Graph node allocation size overflow!");
		throw std::length_error("Graph node allocation size overflow");
	}

	node* replacement = static_cast<node *>(malloc(node_bytes));
	if (!replacement)
	{
		report_error(error_function, "Not enough memory!");
		throw std::bad_alloc();
	}
	memcpy(replacement, nodes_old,
	       static_cast<std::size_t>(node_num)*sizeof(node));

	for (arc* a=arcs; a<arc_last; ++a)
	{
		const std::ptrdiff_t head_offset = a->head-nodes_old;
		a->head = replacement+head_offset;
	}

	free(nodes_old);
	nodes = replacement;
	node_last = nodes + node_num;
	node_max = nodes + node_num_max;
}

template <typename captype, typename tcaptype, typename flowtype> 
	void Graph<captype,tcaptype,flowtype>::reallocate_arcs()
{
	const int old_arc_num_max = static_cast<int>(arc_max - arcs);
	const int arc_num = static_cast<int>(arc_last - arcs);
	std::size_t arc_num_max = static_cast<std::size_t>(old_arc_num_max);
	arc* arcs_old = arcs;

	arc_num_max += arc_num_max / 2;
	if (arc_num_max & 1) arc_num_max ++;
	if (arc_num_max < static_cast<std::size_t>(arc_num)+2)
		arc_num_max = static_cast<std::size_t>(arc_num)+2;
	if (arc_num_max > static_cast<std::size_t>(std::numeric_limits<int>::max()))
	{
		report_error(error_function, "Graph edge capacity is too large!");
		throw std::length_error("Graph edge capacity is too large");
	}

	std::size_t arc_bytes = 0;
	if (!checked_multiply(arc_num_max, sizeof(arc), arc_bytes))
	{
		report_error(error_function, "Graph edge allocation size overflow!");
		throw std::length_error("Graph edge allocation size overflow");
	}

	arc* replacement = static_cast<arc *>(malloc(arc_bytes));
	if (!replacement)
	{
		report_error(error_function, "Not enough memory!");
		throw std::bad_alloc();
	}
	memcpy(replacement, arcs_old,
	       static_cast<std::size_t>(arc_num)*sizeof(arc));

	for (node* i=nodes; i<node_last; ++i)
	{
		if (i->first)
			i->first = replacement+(i->first-arcs_old);
	}
	for (int index=0; index<arc_num; ++index)
	{
		arc* a = replacement+index;
		if (a->next)
			a->next = replacement+(a->next-arcs_old);
		a->sister = replacement+(a->sister-arcs_old);
	}

	free(arcs_old);
	arcs = replacement;
	arc_last = arcs + arc_num;
	arc_max = arcs + arc_num_max;
}
// Hack pour garder les sources d'Olga...
#include "maxflow.inc"
#include "instances.inc"
