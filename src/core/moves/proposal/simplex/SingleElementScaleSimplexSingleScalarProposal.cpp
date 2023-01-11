#include "SingleElementScaleSimplexSingleScalarProposal.h"

#include <stddef.h>
#include <cmath>

#include "RandomNumberFactory.h"
#include "RandomNumberGenerator.h"
#include "RbConstants.h"
#include "Cloneable.h"
#include "StochasticNode.h"

namespace RevBayesCore { class DagNode; }

using namespace RevBayesCore;

/**
 * Constructor
 *
 * Here we simply allocate and initialize the Proposal object.
 */
SingleElementScaleSimplexSingleScalarProposal::SingleElementScaleSimplexSingleScalarProposal( StochasticNode<Simplex> *n, double l, double p) : Proposal(p),
simplex ( n ),
lambda( l )
{
    // tell the base class to add the node
    addNode( simplex );
    
    scalar_added = false;
    
}


void SingleElementScaleSimplexSingleScalarProposal::addIndex( size_t v )
{
    if (v < simplex->getValue().size())
    {
        indices.insert(v);
    }
}


void SingleElementScaleSimplexSingleScalarProposal::addScalar( StochasticNode<double> *v )
{
    
    scalar = v;
    addNode( scalar );
    scalar_added = true;
}


/**
 * The cleanProposal function may be called to clean up memory allocations after AbstractMove
 * decides whether to accept, reject, etc. the proposed value.
 *
 */
void SingleElementScaleSimplexSingleScalarProposal::cleanProposal( void )
{
    if ( scalar_added == true && failed == false )
    {
        RbOrderedSet<DagNode*> affected;
        simplex->initiatefindUniqueDescendants( affected );
        
        for (RbOrderedSet<DagNode*>::iterator it = affected.begin(); it != affected.end(); ++it)
        {
            if ((*it)->getDagNodeType() == RevBayesCore::DagNode::DETERMINISTIC && (*it)->getNumberOfElements() == stored_simplex.size())
            {
                (*it)->clearTouchedElementIndices();
            }
        }
    }
}

/**
 * The clone function is a convenience function to create proper copies of inherited objected.
 * E.g. a.clone() will create a clone of the correct type even if 'a' is of derived type 'b'.
 *
 * \return A new copy of the proposal.
 */
SingleElementScaleSimplexSingleScalarProposal* SingleElementScaleSimplexSingleScalarProposal::clone( void ) const
{
    
    return new SingleElementScaleSimplexSingleScalarProposal( *this );
}


/**
 * Get Proposals' name of object
 *
 * \return The Proposals' name.
 */
const std::string& SingleElementScaleSimplexSingleScalarProposal::getProposalName( void ) const
{
    static std::string name = "SingleElementScaleSimplexSingleScalar";
    
    return name;
}


double SingleElementScaleSimplexSingleScalarProposal::getProposalTuningParameter( void ) const
{
    return lambda;
}


/**
 * Perform the proposal.
 *
 * A Beta-simplex proposal randomly changes some values of a simplex, although the other values
 * change too because of the renormalization.
 * First, some random indices are drawn. Then, the proposal draws a new somplex
 *   u ~ Beta(val[index] * lambda)
 * where lambda is the tuning parameter.The new value is set to u.
 * The simplex is then renormalized.
 *
 * \return The hastings ratio.
 */
double SingleElementScaleSimplexSingleScalarProposal::doProposal( void )
{
    // reset flag
    failed = false;
    
    // Get random number generator
    RandomNumberGenerator* rng     = GLOBAL_RNG;
    
    // now we store all necessary values
    stored_simplex = simplex->getValue();
    const RbVector<double>& simplex_current = simplex->getValue();
    RbVector<double> simplex_new = simplex_current;
    
    size_t chosen_index;
    if ( indices.size() > 0 )
    {
        std::set<size_t>::iterator it = indices.begin();
        if (indices.size() > 1)
        {
            size_t index = size_t( floor(rng->uniform01() * double(indices.size())) );
            std::advance(it, index);
        }
        chosen_index = *it;
    }
    else
    {
        chosen_index = size_t( floor(rng->uniform01() * double(simplex_current.size())) );
    }
    
    double value_current = simplex_current[chosen_index];
    
    // draw new value
    double u = rng->uniform01();
    double scaling_factor = std::exp( lambda * ( u - 0.5 ) );
    double value_new = value_current * scaling_factor;
    simplex_new[chosen_index] = value_new;
    
    double simplex_sum_new = value_new - value_current + 1.0;
    for (size_t i = 0; i < simplex_new.size(); ++i)
    {
        simplex_new[i] /= simplex_sum_new;
        if ( simplex_new[i] < 1E-100)
        {
            failed = true;
            return RbConstants::Double::neginf;
        }
    }
    
    simplex->setValue( new Simplex(simplex_new), false);
    
    // compute the Hastings ratio
    double lnHastingsratio = log(scaling_factor) - log(simplex_sum_new) * simplex_new.size();
    
    if (scalar_added == true)
    {
        stored_scalar = scalar->getValue();
        const double& scalar_current = scalar->getValue();
        double scalar_new = scalar_current * simplex_sum_new;
        lnHastingsratio += log( simplex_sum_new );
        
        scalar->setValue(new double( scalar_new ), false);
        
        RbOrderedSet<DagNode*> affected;
        simplex->initiatefindUniqueDescendants( affected );
        
        for (RbOrderedSet<DagNode*>::iterator it = affected.begin(); it != affected.end(); ++it)
        {
            if ((*it)->getDagNodeType() == RevBayesCore::DagNode::DETERMINISTIC && (*it)->getNumberOfElements() == simplex_new.size())
            {
                (*it)->addTouchedElementIndex(chosen_index);
            }
        }
    }

    return lnHastingsratio;
    
}


/**
 *
 */
void SingleElementScaleSimplexSingleScalarProposal::prepareProposal( void )
{
    
}


/**
 * Print the summary of the Proposal.
 *
 * The summary just contains the current value of the tuning parameter.
 * It is printed to the stream that it passed in.
 *
 * \param[in]     o     The stream to which we print the summary.
 */
void SingleElementScaleSimplexSingleScalarProposal::printParameterSummary(std::ostream &o, bool name_only) const
{
    
    o << "lambda = ";
    if (name_only == false)
    {
        o << lambda;
    }
    
}


/**
 * Reject the Proposal.
 *
 * Since the Proposal stores the previous value and it is the only place
 * where complex undo operations are known/implement, we need to revert
 * the value of the variable/DAG-node to its original value.
 */
void SingleElementScaleSimplexSingleScalarProposal::undoProposal( void )
{
    
    // we undo the proposal only if it didn't fail
    if ( failed == false )
    {
        // undo the proposal
        simplex->setValue( new Simplex(stored_simplex), false);
        
        if (scalar_added == true)
        {
            scalar->setValue(new double( stored_scalar ), false);
            
            RbOrderedSet<DagNode*> affected;
            simplex->initiatefindUniqueDescendants( affected );
            
            for (RbOrderedSet<DagNode*>::iterator it = affected.begin(); it != affected.end(); ++it)
            {
                if ((*it)->getDagNodeType() == RevBayesCore::DagNode::DETERMINISTIC && (*it)->getNumberOfElements() == stored_simplex.size())
                {
                    (*it)->clearTouchedElementIndices();
                }
            }
        }
    }
    
}


/**
 * Swap the current variable for a new one.
 *
 * \param[in]     oldN     The old variable that needs to be replaced.
 * \param[in]     newN     The new RevVariable.
 */
void SingleElementScaleSimplexSingleScalarProposal::swapNodeInternal(DagNode *oldN, DagNode *newN)
{
    
    if (oldN == simplex)
    {
        simplex = static_cast<StochasticNode<Simplex>* >(newN) ;
    }
    else if ( scalar_added == true && oldN == scalar )
    {
        scalar = static_cast<StochasticNode<double>* >(newN);
    }
    
}


void SingleElementScaleSimplexSingleScalarProposal::setProposalTuningParameter(double tp)
{
    lambda = tp;
}


/**
 * Tune the Proposal to accept the desired acceptance ratio.
 *
 * The acceptance ratio for this Proposal should be around 0.44.
 * If it is too large, then we increase the proposal size,
 * and if it is too small, then we decrease the proposal size.
 */
void SingleElementScaleSimplexSingleScalarProposal::tune( double rate )
{
    
    double p = this->targetAcceptanceRate;
    if ( rate > p )
    {
        lambda /= (1.0 + ((rate - p)/(1.0 - p)) );
    }
    else
    {
        lambda *= (2.0 - rate/p);
    }
    
}

