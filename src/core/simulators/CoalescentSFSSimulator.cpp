#include "CoalescentSFSSimulator.h"
#include "DistributionExponential.h"
#include "DistributionPoisson.h"
#include "MpiUtilities.h"
#include "ProgressBar.h"
#include "RandomNumberFactory.h"
#include "RandomNumberGenerator.h"

#ifdef RB_MPI
#include <mpi.h>
#include <thread>
#include <chrono>
#endif

#include <unordered_set>

using namespace RevBayesCore;

CoalescentSFSSimulator::CoalescentSFSSimulator(const std::vector<double>& p, const std::vector<double>& cp, double gt, double mr, const std::string& ploidy) :
population_sizes( p ),
change_points( cp ),
generation_time( gt ),
mutation_rate( mr ),
ploidy_factor( 1.0 )
{
    
    if ( ploidy == "diploid" )
    {
        ploidy_factor = 2.0;
    }
    
}


CoalescentSFSSimulator* CoalescentSFSSimulator::clone( void ) const
{
    return new CoalescentSFSSimulator( *this );
}



RbVector<long>* CoalescentSFSSimulator::simulateSFS( long sample_size, long reps ) const
{
    
    RbVector<long>* sfs = new RbVector<long>(sample_size+1);
    
    RandomNumberGenerator* rng = GLOBAL_RNG;
    
#ifdef RB_MPI
    // forward the rng for different processes
    for ( size_t i=active_PID; i<pid; ++i)
    {
        // we fast forward 7 times, just to be sure
        rng->uniform01();
        rng->uniform01();
        rng->uniform01();
        rng->uniform01();
        rng->uniform01();
        rng->uniform01();
        rng->uniform01();
    }
#endif
    
    size_t reps_this_process = reps;
#ifdef RB_MPI
    reps_this_process = reps / num_processes;
#endif
    
    // start the progress bar
    ProgressBar progress = ProgressBar(reps_this_process, 0);
    
    // Print progress bar (68 characters wide)
    progress.start();
    
    std::vector<double> ages    = std::vector<double>(2*sample_size-1, 0);
    std::vector<size_t> parents = std::vector<size_t>(2*sample_size-1, -1);
    std::vector<std::vector<size_t> > children = std::vector<std::vector<size_t> >(2*sample_size-1, std::vector<size_t>(2,-1));
    
    std::vector<long> tip_state = std::vector<long>(sample_size, 0);
        
    for (size_t r=0; r<reps_this_process; ++r)
    {
        
        // initialize the active lineages, which are all currently available samples
        std::unordered_set<size_t> active_lineages;
        for (size_t i=0; i<sample_size; ++i)
        {
            active_lineages.insert( i );
        }
        
        // initialize the current time
        double current_time = 0;
        
        // now start simulating coalescent events
        size_t next_parent = sample_size;
        for ( size_t num_active=sample_size; num_active>1; --num_active )
        {
            double next_coalescent_time = simulateCoalescentTime( current_time, num_active, rng );
            
            // randomly pick the two lineages to coalesce
            size_t left_index = size_t(rng->uniform01() * num_active);
            std::unordered_set<size_t>::iterator left_it = active_lineages.begin();
            std::advance(left_it, left_index);
            size_t left = *left_it;
            
            // remove the left individual
            active_lineages.erase(left_it);
            
            // choose the right individual
            size_t right_index = size_t(rng->uniform01() * (num_active-1));
            std::unordered_set<size_t>::iterator right_it = active_lineages.begin();
            std::advance(right_it, right_index);
            size_t right = *right_it;
            
            // remove the right individual
            active_lineages.erase(right_it);
            
            // set the parents
            parents[left]  = next_parent;
            parents[right] = next_parent;
            
            // set the childred
            children[next_parent][0] = left;
            children[next_parent][1] = right;
            
            // set the age
            ages[next_parent] = next_coalescent_time;
            
            // add the parent to our set
            active_lineages.insert( next_parent );
            
            // increase the index to the next parent
            next_parent++;
            
            // set the current time to the latest event
            current_time = next_coalescent_time;
        }
        
        // now simulate the mutation
        size_t root_index = 2*sample_size-2;
        long root_state = 0;
        size_t num_mutations = simulateMutations(root_index, root_state, children, ages, tip_state, rng);
        
        size_t obs_state = 0;
        for (size_t tip=0; tip<sample_size; ++tip)
        {
            obs_state += tip_state[tip];
        }
        
        ++(*sfs)[obs_state];
        
        progress.update(r);
    }
    progress.finish();
    
    
#ifdef RB_MPI
    MPI_Barrier( MPI_COMM_WORLD );
    
    // create a copy of the transition probability matrix
    RbVector<long> sfs_backup = RbVector<long>( *sfs );

    // we only need to send message if there is more than one process
    if ( num_processes > 1 )
    {
        std::vector< long > this_sfs = std::vector<long>(sample_size+1,0.0);

        for (size_t i=active_PID; i<active_PID+num_processes; ++i)
        {
            
            if ( pid == i )
            {
                this_sfs = tpv_sfs;
            }
                
            MPI_Bcast(&this_sfs[0], sample_size+1, MPI_INT, pid, MPI_COMM_WORLD);
                
            if ( pid != i )
            {
                for ( size_t k=0; k<=sample_size; ++k )
                {
                    (*sfs)[k] += this_sfs[k];
                }
            } // end-if non-sending process to add the transition probabilities
            
        } // end-for over all processes
        
    } // end-if there are more than one process
#endif

    
    return sfs;
}

double CoalescentSFSSimulator::simulateCoalescentTime(double current_age, size_t num_active, RandomNumberGenerator *rng) const
{
    double coalescent_time = current_age;
    size_t num_intervals = change_points.size();
    
    size_t current_interval = 0;
    while ( current_interval < num_intervals && current_age > change_points[current_interval] )
    {
        ++current_interval;
    }
    
    bool valid = false;
    do
    {
        double num_pairs = num_active * (num_active-1) / 2.0;
        double theta = population_sizes[current_interval] * ploidy_factor;
        double lambda = num_pairs / theta;
        double u = RbStatistics::Exponential::rv( lambda, *rng);
        coalescent_time += u;
        valid = current_interval == num_intervals || coalescent_time < change_points[current_interval];
        
        if ( valid == false )
        {
            // If we cross a theta window, the number of active lineages changes
            // or the pop size changes, and it is necessary to discard any "excess" time,
            // which is drawn from an incorrect distribution.
            coalescent_time = change_points[current_interval];
            ++current_interval;
        }
    } while ( valid == false );
    
    return coalescent_time;
}



size_t CoalescentSFSSimulator::simulateMutations(size_t current_index, long current_state, const std::vector<std::vector<size_t> > &children, const std::vector<double> &ages, std::vector<long> &tip_states, RandomNumberGenerator* rng) const
{
    size_t sample_size = tip_states.size();
    size_t num_mutations = 0;
    
    // treat according if this is a tip
    if ( current_index < sample_size )
    {
        tip_states[current_index] = current_state;
    }
    else
    {
        size_t left = children[current_index][0];
        double left_branch_length = ages[current_index] - ages[left];
        size_t left_num_mutations = RbStatistics::Poisson::rv( mutation_rate*left_branch_length, *rng );

        // initialize the left state with the current state
        long left_state = current_state;
        
        // only flip the state if the number of mutations is odd
        if ( left_num_mutations % 2 == 1 )
        {
            left_state = ( left_state == 0 ? 1 : 1 );
        }
        simulateMutations(left, left_state, children, ages, tip_states, rng);
        
        
        // now simulate for the right child branch
        size_t right = children[current_index][1];
        double right_branch_length = ages[current_index] - ages[right];
        size_t right_num_mutations = RbStatistics::Poisson::rv( mutation_rate*right_branch_length, *rng );

        // initialize the right state with the current state
        long right_state = current_state;
        
        // only flip the state if the number of mutations is odd
        if ( right_num_mutations % 2 == 1 )
        {
            right_state = ( right_state == 0 ? 1 : 1 );
        }
        simulateMutations(right, right_state, children, ages, tip_states, rng);
        
        
        // finally, add the number of mutations together
        num_mutations = left_num_mutations + right_num_mutations;
    }
    
    
    return num_mutations;;
}

