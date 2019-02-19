/*=============================================================================
 * Hugo Raguet 2018
 *===========================================================================*/
#include <cmath>
#include "../include/cut_pursuit_d1.hpp"
#include "../include/omp_num_threads.hpp"

#define ZERO ((real_t) 0.0)
#define ONE ((real_t) 1.0)
#define COOR_WEIGHTS_(d) (coor_weights ? coor_weights[(d)] : ONE)

#define TPL template <typename real_t, typename index_t, typename comp_t>
#define CP_D1 Cp_d1<real_t, index_t, comp_t>

using namespace std;

TPL CP_D1::Cp_d1(index_t V, index_t E, const index_t* first_edge,
    const index_t* adj_vertices, size_t D, D1p d1p)
    : Cp<real_t, index_t, comp_t>(V, E, first_edge, adj_vertices), D(D),
    d1p(d1p)
{
    rX = last_rX = nullptr;
    coor_weights = nullptr;
}

TPL CP_D1::~Cp_d1(){ free(rX); free(last_rX); }

TPL void CP_D1::set_edge_weights(const real_t* edge_weights,
    real_t homo_edge_weight, const real_t* coor_weights)
{
    Cp<real_t, index_t, comp_t>::set_edge_weights(edge_weights,
        homo_edge_weight);
    this->coor_weights = coor_weights;
}

TPL real_t* CP_D1::get_reduced_values(){ return rX; }

TPL void CP_D1::set_reduced_values(real_t* rX){ this->rX = rX; }

TPL void CP_D1::free_comp_values(){ free(rX); rX = nullptr; }

TPL void CP_D1::copy_last_comp_values()
{
    last_rX = (real_t*) malloc_check(sizeof(real_t)*V*D);
    for (size_t i = 0; i < rV*D; i++){ last_rX[i] = rX[i]; }
}

TPL void CP_D1::free_last_comp_values(){ free(last_rX); last_rX = nullptr; }

TPL void CP_D1::resize_comp_values()
{ rX = (real_t*) realloc_check(rX, sizeof(real_t)*D*rV); }

TPL bool CP_D1::is_almost_equal(comp_t ru, comp_t rv)
{
    real_t dif = ZERO, ampu = ZERO, ampv = ZERO;
    real_t *rXu = rX + ru*D;
    real_t *rXv = rX + rv*D;
    for (size_t d = 0; d < D; d++){
        if (d1p == D11){
            dif += abs(rXu[d] - rXv[d])*COOR_WEIGHTS_(d);
            ampu += abs(rXu[d])*COOR_WEIGHTS_(d);
            ampv += abs(rXv[d])*COOR_WEIGHTS_(d);
        }else if (d1p == D12){
            dif += (rXu[d] - rXv[d])*(rXu[d] - rXv[d])*COOR_WEIGHTS_(d);
            ampu += rXu[d]*rXu[d]*COOR_WEIGHTS_(d);
            ampv += rXv[d]*rXv[d]*COOR_WEIGHTS_(d);
        }
    }
    real_t amp = ampu > ampv ? ampu : ampv;
    if (d1p == D12){ dif = sqrt(dif); amp = sqrt(amp); }
    if (eps > amp){ amp = eps; }
    return dif <= dif_tol*amp;
}

TPL void CP_D1::compute_merge_chains()
{
    for (size_t re = 0; re < rE; re++){
        comp_t ru = reduced_edges[2*re];
        comp_t rv = reduced_edges[2*re + 1];
        /* get the root of each component's chain */
        while(ru != merge_chains_root[ru]){ ru = merge_chains_root[ru]; }
        while(rv != merge_chains_root[rv]){ rv = merge_chains_root[rv]; }
        if (ru != rv && is_almost_equal(ru, rv)){ merge_components(ru, rv); }
    }
}

TPL real_t CP_D1::compute_graph_d1()
{
    real_t tv = ZERO;
    #pragma omp parallel for schedule(static) NUM_THREADS(2*rE*D, rE) \
        reduction(+:tv)
    for (size_t re = 0; re < rE; re++){
        real_t *rXu = rX + reduced_edges[2*re]*D;
        real_t *rXv = rX + reduced_edges[2*re + 1]*D;
        real_t dif = ZERO;
        for (size_t d = 0; d < D; d++){
            if (d1p == D11){
                dif += abs(rXu[d] - rXv[d])*COOR_WEIGHTS_(d);
            }else if (d1p == D12){
                dif += (rXu[d] - rXv[d])*(rXu[d] - rXv[d])*COOR_WEIGHTS_(d);
            }
        }
        if (d1p == D12){ dif = sqrt(dif); }
        tv += reduced_edge_weights[re]*dif;
    }
    return tv;
}

/**  instantiate for compilation  **/
template class Cp_d1<float, uint32_t, uint16_t>;
template class Cp_d1<double, uint32_t, uint16_t>;
template class Cp_d1<float, uint32_t, uint32_t>;
template class Cp_d1<double, uint32_t, uint32_t>;
