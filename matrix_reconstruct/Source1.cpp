#include <iostream>
#include <memory>
#include <random>
#include <chrono>
#include <algorithm>
#include <iterator>

#include "Header1.h"

using std::shuffle;
using std::advance;
using std::unique_ptr;
using mat_type = short;
using std::endl;
using std::cout;
unique_ptr<mat_type[]> AllocateMatrix(size_t size)
{
    unique_ptr<mat_type[]>temp(new(mat_type[size * size]));
    return temp;
}

void PrintMatrix(char* text, const mat_type* mat, size_t mat_size)
{
    // print matrix
    cout << text << std::endl;
    for (size_t i = 0; i < mat_size; ++i)
    {
        for (size_t j = 0; j < mat_size; ++j)
        {
            std::cout.setf(std::ios::right);
            std::cout.width(6);
            cout << mat[i * mat_size + j] << ',';
        }
        cout << std::endl;
    }
}

int main()
{
    std::list<ushort> R_vecs; // remaining
    std::list<ushort> U_vecs; // used
    std::list<ushort> N_vecs; // not-matched
    std::list<ushort> trials_list; 
    vector<vector<mat_type>> pool;
    std::random_device rd;
    unsigned int seed = 1852752597;//rd();//42;
    cout << "SEED: " << seed << endl;
    size_t mat_size = 100;
    size_t mat_capacity = mat_size * mat_size;
    int min_mat_value = 1;
    int max_mat_value = 100;

    unique_ptr<mat_type[]> mat = AllocateMatrix(mat_size);

    // possible 2 variants for reconstruction (columns & rows)
    unique_ptr<mat_type[]> mat_rec1 = AllocateMatrix(mat_size);
    unique_ptr<mat_type[]> mat_rec2 = AllocateMatrix(mat_size);
// resize pool
    pool.resize(2 * mat_size);
    for (auto &x : pool)
        x.resize(mat_size);


    std::mt19937 gen(seed); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(min_mat_value, max_mat_value);


    // fill it with random
    std::for_each(mat.get(), mat.get() + mat_capacity, [&dis, &gen](mat_type& x) { x = mat_type(dis(gen)); });

    //rows
    for (size_t i = 0; i < mat_size; ++i)
    {
        for (size_t j = 0; j < mat_size; ++j)
            pool[i][j] = mat[i * mat_size + j];
    }

    //columns:
    for (size_t i = 0; i < size_t(mat_size); ++i)
    {
        for (size_t j = 0; j < size_t(mat_size); ++j)
            pool[i +  mat_size][j] = mat[j * mat_size + i];
    }

#if 0
    // print matrix
    PrintMatrix("Matrix", mat.get(), mat_size);

    // print pool
    cout << "pool" << std::endl;
    for (auto &x : pool)
    {
        for (auto &y : x)
        {
            std::cout.setf(std::ios::right);
            std::cout.width(6);
            cout << y << ',';
        }
        cout << std::endl;
    }
#endif


    std::vector<ushort> rnd_idx(mat_size + mat_size);
    std::generate(rnd_idx.begin(), rnd_idx.end(), [n = 0]() mutable { return n++; });

    //shufle it;
    std::shuffle(rnd_idx.begin(), rnd_idx.end(), gen);
    std::copy(rnd_idx.begin(), rnd_idx.end(), std::back_inserter(R_vecs));



    U_vecs.clear();
    N_vecs.clear();
    trials_list.clear();

    size_t  prev_recovered_elems = 0;
    size_t  recovered_elems = 0;
    // this is a counter to count how many times a last parent can be changed prior to roll 1 step back
    size_t trials = 0;
    bool all_vairants_tried = false;
    bool bNextTrial = false;
    // check time
    auto start_time = std::chrono::system_clock::now();
    while (U_vecs.size() != (mat_size * 2))
    {
        if (recovered_elems > prev_recovered_elems) {
            printf("recovered elements: %d\n", recovered_elems);
            prev_recovered_elems = recovered_elems;
        }

        if (R_vecs.empty())
        {
            // signal that this iteration we are going to check next candidate for a parent
                bNextTrial = true;
                // move last from used_vecs to unmantched
                auto badprev = U_vecs.end();
                --badprev;

                if (!all_vairants_tried)
                {
                    // we tried all vectors at the current level; so copy all vectors from unmatched back to remaining
                    R_vecs.splice(R_vecs.begin(), N_vecs);
                    R_vecs.splice(R_vecs.end(), U_vecs, badprev);

                    // reduce number of recovered elems
                    if (0 == recovered_elems)
                        throw("0 == recovered_elems!");
                    --recovered_elems;
                    auto it = trials_list.end();
                    *(--it) +=1;
                }
                else
                {
                    //this is a condtion for prev-prev roll-back
                    trials_list.pop_back();
//                    auto it = trials_list.end();
//                    *(--it) += 1;

                    if (0 == recovered_elems)
                        throw("0 == recovered_elems!");
                    --recovered_elems;
                    N_vecs.splice(N_vecs.end(), U_vecs, badprev);

                }
        }
        else
        {
            ushort idx2test = R_vecs.front();
            size_t elem_to_match = recovered_elems >> 1;
            size_t num_to_match = (recovered_elems + 1) >> 1;
            bool match(true);

            if (!U_vecs.empty())
            {
                auto it = U_vecs.end();
                ushort idx;
                auto& tst_vec = pool[idx2test];
                for (int i = num_to_match - 1; i >=0 ; --i)
                {
                    --it;
                    idx = *it;
                    if (tst_vec[i] != pool[idx][elem_to_match])
                    {
                        match = false;
                        break;
                    }
                    if (it != U_vecs.begin())
                        --it;
                }
            }

            if (match)
            {
//                if (trials <= (mat_size * 2) - U_vecs.size() + 1)
                {
                    ++recovered_elems;
                    // move next element from a pool into used pool
                    U_vecs.splice(U_vecs.end(), R_vecs, R_vecs.begin());
                    R_vecs.splice(R_vecs.end(), N_vecs);
                    if (!bNextTrial)
                        trials_list.push_back(0);
                    else
                        bNextTrial = false;
                }
            }
            else
            {
                // otherwise keep it as unmatched
                N_vecs.splice(N_vecs.end(), R_vecs, R_vecs.begin());
                //++trials;
            }
        }

        //this is a condition for parent
        all_vairants_tried = ((mat_size * 2) - U_vecs.size() + 1) == trials_list.back();
    }

    auto end_time = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> diff = end_time - start_time;

    cout << "recovered. Took: " << diff.count() << " ms." << std::endl;


    int row_idx = 1;
    //for (auto &x : U_vecs)
    //{
    //    if (row_idx & 1)
    //    {
    //        for (auto &y : pool[x])
    //        {
    //            std::cout.setf(std::ios::right);
    //            std::cout.width(6);
    //            cout << y << ',';
    //        }
    //        cout << std::endl;
    //    }
    //    row_idx += 1;
    //}


    //recover: takes odd only
    row_idx = 1;
    size_t i = 0;
    for (auto &x : U_vecs)
    {
        if (row_idx & 1)
        {
            for (size_t j = 0; j < mat_size; ++j)
                mat_rec1[i * mat_size + j] = pool[x][j];
            ++i;
        }
        row_idx += 1;
    };


    //recover: takes even only
    row_idx = 0;
    i = 0;
    for (auto &x : U_vecs)
    {
        if (row_idx & 1)
        {
            for (size_t j = 0; j < mat_size; ++j)
                mat_rec2[i * mat_size + j] = pool[x][j];
            ++i;
        }
        row_idx += 1;
    };

    bool MatRec1_eq(false);
    bool MatRec2_eq(false);
    MatRec1_eq = std::equal(mat.get(), mat.get() + mat_capacity, mat_rec1.get());
    MatRec2_eq = std::equal(mat.get(), mat.get() + mat_capacity, mat_rec2.get());

    cout << "comparison results: " << (MatRec1_eq || MatRec2_eq ? "Recovered" : "Not recovered") << std::endl;
    cout << "comparison results: " << "MatRec1_eq: " << MatRec1_eq << "; MatRec2_eq: " << MatRec2_eq << std::endl;


    //// checks that it matches:
    //// 1. checks that all even (odd) contains either rows or columns

    ////rows
    //for (size_t i = 0; i < mat_size; ++i)
    //{
    //    for (size_t j = 0; j < mat_size; ++j)
    //        pool[i][j] = mat[i * mat_size + j];
    //}









};