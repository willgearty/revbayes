#ifndef AdaptiveReversibleJumpProposal_H
#define AdaptiveReversibleJumpProposal_H

#include <set>
#include <string>

#include "Proposal.h"
#include "StochasticNode.h"

namespace RevBayesCore {
    
    /**
     * The adaptive reversible jump proposal to switch between two elements of an RJ-Mixture.
     *
     * @copyright Copyright 2009-
     * @author The RevBayes Development Core Team (Sebastian Hoehna)
     * @since 2022-12-05, version 1.2
     *
     */
    class AdaptiveReversibleJumpProposal : public Proposal {
        
    public:
        AdaptiveReversibleJumpProposal( StochasticNode<double> *n, size_t n0, size_t c0, size_t m);                                                             //!<  constructor
        AdaptiveReversibleJumpProposal( const AdaptiveReversibleJumpProposal &p );
        virtual ~AdaptiveReversibleJumpProposal();
        
        AdaptiveReversibleJumpProposal&      operator=(const AdaptiveReversibleJumpProposal& p);
        
        // Basic utility functions
        void                                cleanProposal(void);                                                                //!< Clean up proposal
        AdaptiveReversibleJumpProposal*     clone(void) const;                                                                  //!< Clone object
        double                              doProposal(void);                                                                   //!< Perform proposal
        const std::string&                  getProposalName(void) const;                                                        //!< Get the name of the proposal for summary printing
        double                              getProposalTuningParameter(void) const;
        void                                prepareProposal(void);                                                              //!< Prepare the proposal
        void                                printParameterSummary(std::ostream &o, bool name_only) const;                                       //!< Print the parameter summary
        void                                setProposalTuningParameter(double tp);
        void                                tune(double r);                                                                     //!< Tune the proposal to achieve a better acceptance/rejection ratio
        void                                undoProposal(void);                                                                 //!< Reject the proposal
        
    protected:
        
        void                                swapNodeInternal(DagNode *oldN, DagNode *newN);                                     //!< Swap the DAG nodes on which the Proposal is working on
        
        
    private:
        // parameters
        
        StochasticNode<double>*             variable;                                                                           //!< The variable the Proposal is working on
        double                              stored_value;                                                                       //!< The stored value of the Proposal used for rejections.
        size_t                              stored_index;
        
        size_t                              wait_before_learning;                                                               //!< How long to wait before tracking empirical covariances
        size_t                              wait_before_using;                                                                  //!< How long to wait before using the empirical covariances
        size_t                              num_tried;                                                                          //!< How many times has this move been used?
        size_t                              updates;                                                                            //!< How many updates have been tried?
        size_t                              max_updates;                                                                        //!< How many updates until we stop monitoring the covariances?

    };
    
}


#endif

