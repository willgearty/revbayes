#ifndef AbstractFossilizedBirthDeathProcess_H
#define AbstractFossilizedBirthDeathProcess_H

#include "HomologousDiscreteCharacterData.h"
#include "MatrixReal.h"
#include "NaturalNumbersState.h"
#include "RbVector.h"
#include "TypedDagNode.h"
#include "TypedDistribution.h"

#include <vector>
#include <set>

namespace RevBayesCore {
    
    class Clade;
    class Taxon;

    /**
     * @brief Abstract piecewise-constant fossilized birth-death range process.
     *
     * The piecewise-constant fossilized birth-death range process has constant rates for each time interval.
     * At the end of each time interval there may be an abrupt rate-shift (jump) for each
     * of the rates. Additionally, there may be sampling at the end of each interval.
     * Finally, fossils are sampled with rate psi, the others (fossils and extant taxa) are
     * sampled at sampling times (including the present).
     *
     * We assume that the rate vectors have one more element than the rate-change vectors.
     * Thus, one rate-change means always two interval, two rate-changes three interval, and so on.
     *
     *
     * @copyright Copyright 2009-
     * @author The RevBayes Development Core Team (Sebastian Hoehna)
     * @since 2014-03-18, version 1.0
     *
     */
    class AbstractFossilizedBirthDeathProcess {
        
    public:
        AbstractFossilizedBirthDeathProcess(const DagNode *speciation,
                                            const DagNode *extinction,
                                            const DagNode *psi,
                                            const TypedDagNode<double>* rho,
                                            const TypedDagNode<RbVector<double> > *times,
                                            const std::vector<Taxon> &taxa,
                                            bool complete,
                                            bool extended,
                                            double resampling);  //!< Constructor

        virtual ~AbstractFossilizedBirthDeathProcess(){};

    protected:
        virtual void                                    updateStartEndTimes() = 0;
        virtual double                                  computeLnProbabilityRanges(bool force = false);

        // Parameter management functions
        void                                            swapParameterInternal(const DagNode *oldP, const DagNode *newP);                //!< Swap a parameter

        // helper functions
        size_t                                          findIndex(double t) const;                             //!< Find the index so that times[index-1] < t < times[index]
        double                                          p(size_t i, double t) const;
        virtual double                                  q(size_t i, double t, bool tilde = false) const;

        virtual void                                    prepareProbComputation();

        void                                            keepSpecialization(DagNode *toucher);
        void                                            restoreSpecialization(DagNode *toucher);
        void                                            touchSpecialization(DagNode *toucher, bool touchAll);

        void                                            redrawAges(size_t i, bool force = false);

        std::vector<Taxon>                              fbd_taxa;                                              //!< Taxa that will be attached to new simulated trees.

        size_t                                          num_intervals;

        // members
        const TypedDagNode<double >*                    homogeneous_lambda;                                    //!< The homogeneous speciation rates.
        const TypedDagNode<RbVector<double> >*          heterogeneous_lambda;                                  //!< The heterogeneous speciation rates.
        const TypedDagNode<double >*                    homogeneous_mu;                                        //!< The homogeneous speciation rates.
        const TypedDagNode<RbVector<double> >*          heterogeneous_mu;                                      //!< The heterogeneous speciation rates.
        const TypedDagNode<double >*                    homogeneous_psi;                                       //!< The homogeneous speciation rates.
        const TypedDagNode<RbVector<double> >*          heterogeneous_psi;                                     //!< The heterogeneous speciation rates.
        const TypedDagNode<double >*                    homogeneous_rho;                                       //!< The homogeneous speciation rates.
        const TypedDagNode<RbVector<double> >*          timeline;                                              //!< The times of the instantaneous sampling events.

        std::vector<const DagNode*>                     range_parameters;

        std::vector<double>                     birth;
        std::vector<double>                     death;
        std::vector<double>                     fossil;
        std::vector<double>                     times;

        std::vector<double>                     b_i;
        std::vector<double>                     d_i;
        std::vector<double>                     stored_d_i;
        std::vector<double>                     o_i;
        std::vector<double>                     y_i;

        double                                  origin;

        std::vector<double>                     q_i;
        std::vector<double>                     q_tilde_i;
        std::vector<double>                     p_i;

        std::vector<double>                     Psi;
        std::vector<double>                     stored_Psi;

        std::vector<double>                     tau1;
        std::vector<double>                     stored_tau1;

        std::vector<double>                     partial_likelihood;
        std::vector<double>                     stored_likelihood;

        std::vector<bool>                       dirty_psi;
        std::vector<bool>                       dirty_taxa;

        bool                                    complete;
        bool                                    extended;
        bool                                    touched;

        double                                  resampling;
    };
}

#endif