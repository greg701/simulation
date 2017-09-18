/*
 $Id: gspan.cpp,v 1.8 2004/05/21 09:27:17 taku-ku Exp $;

 Copyright (C) 2004 Taku Kudo, All rights reserved.
 This is free software with ABSOLUTELY NO WARRANTY.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 02111-1307, USA
 */
#include "gspan.h"
#include <iterator>

#include <stdlib.h>
#include <unistd.h>

namespace GSPAN {

gSpan::gSpan(void) {

}
#ifdef DEBUG1
void gSpan::outputmp4(Projected_map4 mp, std::string prefix, std::string suffix,
		std::string indent) {
	for (Projected_iterator4 it = mp.begin(); it != mp.end(); ++it) {
		char first[12];
		sprintf(first, "%d", it->first);
		outputmp3(it->second, prefix + first + ",", "]", indent);
	}
}
void gSpan::outputmp3(Projected_map3 mp, std::string prefix, std::string suffix,
		std::string indent) {
	for (Projected_iterator3 it = mp.begin(); it != mp.end(); ++it) {
		char first[12];
		sprintf(first, "%d", it->first);
		outputmp2(it->second, prefix + first + ",", "]", indent);
	}
}
void gSpan::outputmp2(Projected_map2 mp, std::string prefix, std::string suffix,
		std::string indent) {
	for (Projected_iterator2 it = mp.begin(); it != mp.end(); ++it) {
		char first[12];
		sprintf(first, "%d", it->first);
		outputmp1(it->second, prefix + first + ",", "]", indent);
	}
}
void gSpan::outputmp1(Projected_map1 mp, std::string prefix, std::string suffix,
		std::string indent) {
	for (Projected_iterator1 it = mp.begin(); it != mp.end(); ++it) {
		char first[12];
		sprintf(first, "%c", it->first);
		outputProjected(it->second, prefix + first, "]", indent);
	}
}
void gSpan::outputProjected(Projected match, std::string prefix,
		std::string suffix, std::string indent) {
	*os << prefix << "]=";
	for (Projected::iterator it = match.begin(); it != match.end(); ++it) {
		*os << it->id << ":(" << it->edge->from << "," << it->edge->to << "),";
	}
	*os << std::endl;
}

void gSpan::outputdfscode(DFSCode &code) {
	for (DFSCode::iterator it = code.begin(); it != code.end(); ++it) {
		printf("(%d,%d,%d,%d,%d,%c)", it->from, it->to, it->fromlabel,
				it->elabel, it->tolabel, it->src);
	}
	printf("\n");
}
#endif

std::istream &gSpan::read(std::istream &is) {
	Graph g(directed);
	while (true) {
		g.read(is);
		if (g.empty())
			break;
		TRANS.push_back(g);
//		TRACE("trans#%d\n", TRANS.size());
	}
	return is;
}

std::map<unsigned int, unsigned int> gSpan::support_counts(
		Projected &projected) {
	std::map<unsigned int, unsigned int> counts;

	for (Projected::iterator cur = projected.begin(); cur != projected.end();
			++cur) {
		counts[cur->id] += 1;
	}

	return (counts);
}

unsigned int gSpan::support(Projected &projected) {
	unsigned int oid = 0xffffffff;
	unsigned int size = 0;

	for (Projected::iterator cur = projected.begin(); cur != projected.end();
			++cur) {
		if (oid != cur->id) {
			++size;
		}
		oid = cur->id;
	}

	return size;
}

/* Special report function for single node graphs.
 */
void gSpan::report_single(Graph &g, unsigned int sup) {
//	TRACE("maxpat_max=%u,maxpat_min=%u,g.size()=%lu\n", maxpat_max, maxpat_min,
//			g.size());
	if (maxpat_max > maxpat_min && g.size() > maxpat_max)
		return;
	if (maxpat_min > 0 && g.size() < maxpat_min)
		return;

	if (enc == false) {
#ifndef DIRECTED
		if (where == false)
#endif
		*os << "t # " << ID << " * " << sup;
		*os << '\n';

		g.write(*os);
		*os << '\n';
	} else {
		std::cerr << "report_single not implemented for non-Matlab calls"
				<< std::endl;
	}
	ID++;
}

void gSpan::report(Projected &projected, unsigned int sup) {
	/* Filter to small/too large graphs.
	 */
	if (maxpat_max > maxpat_min && DFS_CODE.nodeCount() > maxpat_max)
		return;
	if (maxpat_min > 0 && DFS_CODE.nodeCount() < maxpat_min)
		return;
#ifndef DIRECTED
	if (where) {
		*os << "<pattern>\n";
		*os << "<id>" << ID << "</id>\n";
		*os << "<support>" << sup << "</support>\n";
		*os << "<what>";
	}
#endif

	if (!enc) {
		Graph g(directed);
		DFS_CODE.toGraph(g);
#ifndef DIRECTED
		if (!where)
#endif
		*os << "t # " << ID << " * " << sup;

		*os << '\n';
		g.write(*os);
	} else {
		if (!where)
			*os << '<' << ID << ">    " << sup << " [";

		DFS_CODE.write(*os);
		if (!where)
			*os << ']';
	}

	if (where) {
		*os << "Occurrent In Trans:";

		for (Projected::iterator cur = projected.begin();
				cur != projected.end(); ++cur) {
			if (cur != projected.begin())
				*os << ' ';
			*os << cur->id;
			printf("(%d,%d)", cur->edge->from, cur->edge->to);
		}
#ifdef DIRECTED
#else
		*os << "</where>\n</pattern>";
#endif
	}
#ifdef DIRECTED
	*os << "\n\n";
#else
	*os << '\n';
#endif
	++ID;
}

/* Recursive subgraph mining function (similar to subprocedure 1
 * Subgraph_Mining in [Yan2002]).
 */
void gSpan::project(Projected &projected) {

#ifdef DEBUG1
	printf(
			"====================================================================\n");
	TRACE("Report dfs_code\n");
	outputdfscode(DFS_CODE);
	TRACE("Report dfs_code\n\n");
#endif
	/* Check if the pattern is frequent enough.
	 */
	unsigned int sup = support(projected);
	if (sup < minsup) {
#ifdef DEBUG1
		TRACE("sup is less than minsup\n");
#endif
		return;
	}

	/* The minimal DFS code check is more expensive than the support check,
	 * hence it is done now, after checking the support.
	 */
	if (is_min() == false) {
		//      *os  << "NOT MIN [";  DFS_CODE.write (*os);  *os << "]" << std::endl;
#ifdef DEBUG1
		TRACE("not a min DFS_CODE, Report Code\n");
		outputdfscode(DFS_CODE);
		TRACE("not a min DFS_CODE, Report Code End\n");
#endif
		return;
	}

	// Output the frequent substructure
	report(projected, sup);

	/* In case we have a valid upper bound and our graph already exceeds it,
	 * return.  Note: we do not check for equality as the DFS exploration may
	 * still add edges within an existing subgraph, without increasing the
	 * number of nodes.
	 */
	if (maxpat_max > maxpat_min && DFS_CODE.nodeCount() > maxpat_max) {
#ifdef DEBUG1
		TRACE("exceed nodes limit\n");
#endif
		return;
	}

	/* We just outputted a frequent subgraph.  As it is frequent enough, so
	 * might be its (n+1)-extension-graphs, hence we enumerate them all.
	 */
	const RMPath &rmpath = DFS_CODE.buildRMPath(); //build by backward traverse.
	int minlabel = DFS_CODE[0].fromlabel; //root vertex label, minlabel of dfscode
	int maxtoc = DFS_CODE[rmpath[0]].to; //max vertexID of dfs code.

#ifdef DIRECTED
	Projected_map4 new_fwd_root; //(fromVid,elabel,toVlabel,src)
	Projected_map3 new_bck_root; //(toVid,elabel,src)
#else

	Projected_map3 new_fwd_root; //(fromVid,elabel,toVlabel)
	Projected_map2 new_bck_root;//(toVid,elabel)==>PDFS
#endif

	EdgeList edges;

	/* Enumerate all possible one edge extensions of the current substructure.
	 */
	for (unsigned int n = 0; n < projected.size(); ++n) {

		unsigned int id = projected[n].id; //transID
		PDFS *cur = &projected[n]; //
		History history(TRANS[id], cur);

		// backward
#ifdef DIRECTED
		for (int i = (int) rmpath.size() - 1; i >= 0; --i) {
			//if(DFS_CODE[rmpath[i]].src==)
			int from, to;
			if (DFS_CODE[rmpath[i]].src == 'l') {
				from = history[rmpath[i]]->from;
			} else if (DFS_CODE[rmpath[i]].src == 'r') {
				from = history[rmpath[i]]->to;
			} else
				assert(false);
			if (DFS_CODE[rmpath[0]].src == 'l') {
				to = history[rmpath[0]]->to;
			} else if (DFS_CODE[rmpath[0]].src == 'r') {
				to = history[rmpath[0]]->from;
			} else
				assert(false);

			Edge *e = get_backward(TRANS[id], history[rmpath[i]],
					history[rmpath[0]], history, from, to);
			if (e)
				if (e->from == from)
					new_bck_root[DFS_CODE[rmpath[i]].from][e->elabel]['r'].push(
							id, e, cur);
				else if (e->from == to)
					new_bck_root[DFS_CODE[rmpath[i]].from][e->elabel]['l'].push(
							id, e, cur);
		}
#else
		for (int i = (int) rmpath.size() - 1; i >= 1; --i) {
			Edge *e = get_backward(TRANS[id], history[rmpath[i]],
					history[rmpath[0]], history);
			if (e)
			new_bck_root[DFS_CODE[rmpath[i]].from][e->elabel].push(id, e,
					cur);
		}
#endif

		// pure forward
		// FIXME: here we pass a too large e->to (== history[rmpath[0]]->to
		// into get_forward_pure, such that the assertion fails.
		//
		// The problem is:
		// history[rmpath[0]]->to > TRANS[id].size()
#ifdef DIRECTED

		int v;
		if (DFS_CODE[rmpath[0]].src == 'l') {
			v = history[rmpath[0]]->to;
		} else if (DFS_CODE[rmpath[0]].src == 'r') {
			v = history[rmpath[0]]->from;
		} else
			assert(false);

		if (get_forward_pure(TRANS[id], v, minlabel, history, edges))
			for (EdgeList::iterator it = edges.begin(); it != edges.end(); ++it)
				if ((*it)->from == v)
					new_fwd_root[maxtoc][(*it)->elabel][TRANS[id][(*it)->to].label]['l'].push(
							id, *it, cur);
				else if ((*it)->to == v)
					new_fwd_root[maxtoc][(*it)->elabel][TRANS[id][(*it)->from].label]['r'].push(
							id, *it, cur);
				else
					assert(false);
#else
		if (get_forward_pure(TRANS[id], history[rmpath[0]], minlabel, history,
						edges))
		for (EdgeList::iterator it = edges.begin(); it != edges.end(); ++it)
		new_fwd_root[maxtoc][(*it)->elabel][TRANS[id][(*it)->to].label].push(
				id, *it, cur);
#endif

		// backtracked forward
#ifdef DIRECTED
		for (int i = 0; i < (int) rmpath.size(); ++i) {

			int v1, v2;
			if (DFS_CODE[rmpath[i]].src == 'l') {
				v1 = history[rmpath[i]]->from;
				v2 = history[rmpath[i]]->to;
			} else if (DFS_CODE[rmpath[i]].src == 'r') {
				v1 = history[rmpath[i]]->to;
				v2 = history[rmpath[i]]->from;
			} else
				assert(false);

			if (get_forward_rmpath(TRANS[id], v1, v2, history[rmpath[i]],
					minlabel, history, edges))
				for (EdgeList::iterator it = edges.begin(); it != edges.end();
						++it)
					if ((*it)->from == v1)
						new_fwd_root[DFS_CODE[rmpath[i]].from][(*it)->elabel][TRANS[id][(*it)->to].label]['l'].push(
								id, *it, cur);
					else if ((*it)->to == v1)
						new_fwd_root[DFS_CODE[rmpath[i]].from][(*it)->elabel][TRANS[id][(*it)->from].label]['r'].push(
								id, *it, cur);
					else
						assert(false);
		}
#else
		for (int i = 0; i < (int) rmpath.size(); ++i)
		if (get_forward_rmpath(TRANS[id], history[rmpath[i]], minlabel,
						history, edges))
		for (EdgeList::iterator it = edges.begin(); it != edges.end();
				++it)
		new_fwd_root[DFS_CODE[rmpath[i]].from][(*it)->elabel][TRANS[id][(*it)->to].label].push(
				id, *it, cur);
#endif
	}

#ifdef DEBUG1
	TRACE("Report extend\n");
	*os << "backward\n";
	outputmp3(new_bck_root);
	*os << "forward \n";
	outputmp4(new_fwd_root);
	TRACE("Report extend end\n");
#endif
	/* Test all extended substructures.
	 *
	 *
	 */

	/* Attention: how could the vertex label be -1, and why ?
	 *
	 * in the DFS_CODE.push operations, sometime the label of vertex is -1, the label -1 indicates
	 * this vertex has been included in the previous DFS_code and the label can be obtained by
	 * traverse the DFS_code
	 *
	 */
	// backward
#ifdef DIRECTED
	for (Projected_iterator3 to = new_bck_root.begin();
			to != new_bck_root.end(); ++to) {
		for (Projected_iterator2 elabel = to->second.begin();
				elabel != to->second.end(); ++elabel) {
			for (Projected_iterator1 src = elabel->second.begin();
					src != elabel->second.end(); ++src) {
				DFS_CODE.push(maxtoc, to->first, -1, elabel->first, -1,
						src->first);
				project(src->second);
				DFS_CODE.pop();
			}
		}
	}
#else
	for (Projected_iterator2 to = new_bck_root.begin();
			to != new_bck_root.end(); ++to) {
		for (Projected_iterator1 elabel = to->second.begin();
				elabel != to->second.end(); ++elabel) {
			DFS_CODE.push(maxtoc, to->first, -1, elabel->first, -1);
			project(elabel->second);
			DFS_CODE.pop();
		}
	}
#endif

	// forward
#ifdef DIRECTED
	for (Projected_riterator4 from = new_fwd_root.rbegin();
			from != new_fwd_root.rend(); ++from) {
		for (Projected_iterator3 elabel = from->second.begin();
				elabel != from->second.end(); ++elabel) {
			for (Projected_iterator2 tolabel = elabel->second.begin();
					tolabel != elabel->second.end(); ++tolabel) {
				for (Projected_iterator1 src = tolabel->second.begin();
						src != tolabel->second.end(); ++src) {
					DFS_CODE.push(from->first, maxtoc + 1, -1, elabel->first,
							tolabel->first, src->first);
					project(src->second);
					DFS_CODE.pop();
				}
			}
		}
	}
#else
	for (Projected_riterator3 from = new_fwd_root.rbegin();
			from != new_fwd_root.rend(); ++from) {
		for (Projected_iterator2 elabel = from->second.begin();
				elabel != from->second.end(); ++elabel) {
			for (Projected_iterator1 tolabel = elabel->second.begin();
					tolabel != elabel->second.end(); ++tolabel) {
				DFS_CODE.push(from->first, maxtoc + 1, -1, elabel->first,
						tolabel->first);
				project(tolabel->second);
				DFS_CODE.pop();
			}
		}
	}
#endif

	return;
}

void gSpan::run(std::istream &is, std::ostream &_os, unsigned int _minsup,
		unsigned int _maxpat_min, unsigned int _maxpat_max, bool _enc,
		bool _where, bool _directed) {
	os = &_os;
	ID = 0;
	minsup = _minsup;
	maxpat_min = _maxpat_min;
	maxpat_max = _maxpat_max;
	enc = _enc;
	where = _where;
	directed = _directed;

	TRACE(
			"Call parameter:\n  (_minsup=%u,_maxpat_min=%u,_maxpat_max=%u,_enc=%d,_where=%d,_directed=%d)\n",
			minsup, maxpat_min, maxpat_max, enc, where, directed);

	read(is);
	run_intern();
}

void gSpan::run_intern(void) {

	//single node pattern
	if (maxpat_min <= 1) {
		for (unsigned int id = 0; id < TRANS.size(); ++id) {
			for (unsigned int nid = 0; nid < TRANS[id].size(); ++nid) {
				if (singleVertex[id][TRANS[id][nid].label] == 0) {
					// number of graphs it appears in
					singleVertexLabel[TRANS[id][nid].label] += 1;
				}
				singleVertex[id][TRANS[id][nid].label] += 1; //(transID,vlabel)
			}
		}

		for (std::map<unsigned int, unsigned int>::iterator it =
				singleVertexLabel.begin(); it != singleVertexLabel.end();
				++it) {
			if ((*it).second < minsup)
				continue;

			unsigned int frequent_label = (*it).first;

			/* Found a frequent node label, report it.
			 */
			Graph g(directed);
			g.resize(1);
			g[0].label = frequent_label;

			report_single(g, (*it).second);
		}
	}

#ifdef DIRECTED

	Projected_map3 root;
	Projected_map4 root4; // (v1label,elabel,v2label,direction)==>PDFS(transID,edge*,0)

	EdgeList edges;

	/*
	 * for hierarchy: transactionsID --> srcVertexId --> ajacentVertexes
	 *
	 * K,V datastructure
	 *(svlabel,elabel,dvlabel)==>array(transID,edge*,prev*)
	 */
	for (unsigned int id = 0; id < TRANS.size(); ++id) {
		Graph &g = TRANS[id];
		for (unsigned int from = 0; from < g.size(); ++from) {
			if (get_forward_root(g, g[from], edges)) {
				for (EdgeList::iterator it = edges.begin(); it != edges.end();
						++it)
					if (g[from].label <= g[(*it)->to].label) //should be less or equal, because l<r
						root4[g[from].label][(*it)->elabel][g[(*it)->to].label]['l'].push(
								id, *it, 0);
					else
						root4[g[(*it)->to].label][(*it)->elabel][g[from].label]['r'].push(
								id, *it, 0);
			}
		}
	}
#ifdef DEBUG1
	TRACE("time is %d\n", 10);
	*os << "[vlabel,elabel,vlabel,src]" << std::endl;
	outputmp4(root4);
	*os << std::endl;
#endif
	/*
	 *
	 * for hierarchy
	 * 		svlabel -->  elabel --> dvlabel --> src
	 *
	 */
	for (Projected_iterator4 fromlabel = root4.begin();
			fromlabel != root4.end(); ++fromlabel) {
		for (Projected_iterator3 elabel = fromlabel->second.begin();
				elabel != fromlabel->second.end(); ++elabel) {
			for (Projected_iterator2 tolabel = elabel->second.begin();
					tolabel != elabel->second.end(); ++tolabel) {
				for (Projected_iterator1 src = tolabel->second.begin();
						src != tolabel->second.end(); src++) {
					DFS_CODE.push(0, 1, fromlabel->first, elabel->first,
							tolabel->first, src->first);
					project(src->second);
					DFS_CODE.pop();
				}
				/*
				 if (tolabel->second.find('r') != tolabel->second.end()) {
				 DFS_CODE.push(0, 1, fromlabel->first, elabel->first,
				 tolabel->first, 'r');
				 project(tolabel->second['r']);
				 DFS_CODE.pop();
				 }
				 if (tolabel->second.find('l') != tolabel->second.end()) {
				 DFS_CODE.push(0, 1, fromlabel->first, elabel->first,
				 tolabel->first, 'l');
				 project(tolabel->second['l']);
				 DFS_CODE.pop();
				 }
				 */
			}
		}
	}
#else
	//(svlabel,elabel,dvlabel)==>PDFS(transID,edge*,0)

	Projected_map3 root;

	EdgeList edges;

	/*
	 * for hierarchy: transactionsID --> srcVertexId --> ajacentVertexes
	 *
	 * K,V datastructure
	 *(svlabel,elabel,dvlabel)==>array(transID,edge*,prev*)
	 */
	for (unsigned int id = 0; id < TRANS.size(); ++id) {
		Graph &g = TRANS[id];
		for (unsigned int from = 0; from < g.size(); ++from) {
			if (get_forward_root(g, g[from], edges)) {
				for (EdgeList::iterator it = edges.begin(); it != edges.end();
						++it)
				root[g[from].label][(*it)->elabel][g[(*it)->to].label].push(
						id, *it, 0);
			}
		}
	}

	/*
	 *
	 * for hierarchy
	 * 		svlabel -->  elabel --> dvlabel
	 *
	 */
	for (Projected_iterator3 fromlabel = root.begin(); fromlabel != root.end();
			++fromlabel) {
		for (Projected_iterator2 elabel = fromlabel->second.begin();
				elabel != fromlabel->second.end(); ++elabel) {
			for (Projected_iterator1 tolabel = elabel->second.begin();
					tolabel != elabel->second.end(); ++tolabel) {
				/* Build the initial two-node graph.  It will be grown
				 * recursively within project.
				 */
				DFS_CODE.push(0, 1, fromlabel->first, elabel->first,
						tolabel->first);
				project(tolabel->second);
				DFS_CODE.pop();
			}
		}
	}
#endif

}

}