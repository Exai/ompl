/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2012, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: Ioan Sucan, Luis G. Torres */

#ifndef OMPL_BASE_OPTIMIZATION_OBJECTIVE_
#define OMPL_BASE_OPTIMIZATION_OBJECTIVE_

#include "ompl/base/SpaceInformation.h"
#include "ompl/util/ClassForward.h"
#include <boost/noncopyable.hpp>
#include <boost/concept_check.hpp>
#include <limits>

namespace ompl
{
    namespace base
    {   
        struct Cost
        {
            explicit Cost(double v = 0.0) : v(v) {}
            double v;
        };

        class Goal;

        typedef boost::function<Cost (const State*, const Goal*)> CostToGoHeuristic;
      
        /// @cond IGNORE
        /** \brief Forward declaration of ompl::base::OptimizationObjective */
        OMPL_CLASS_FORWARD(OptimizationObjective);
        /// @endcond

        /// @cond IGNORE
        OMPL_CLASS_FORWARD(Path);
        /// @endcond	

        /** \class ompl::base::OptimizationObjectivePtr
            \brief A boost shared pointer wrapper for ompl::base::OptimizationObjective */

        /** \brief Abstract definition of optimization objectives.

            \note This implementation has greatly benefited from discussions with <a href="http://www.cs.indiana.edu/~hauserk/">Kris Hauser</a> */
        class OptimizationObjective : private boost::noncopyable
        {
        public:
            /** \brief Constructor. The objective must always know the space information it is part of */
            OptimizationObjective(const SpaceInformationPtr &si) :
                si_(si),
                threshold_(0.0)
	    {
	    }           

            virtual ~OptimizationObjective(void)
            {
            }

            /** \brief Get the description of this optimization objective */
            const std::string& getDescription(void) const
            {
                return description_;
            }

            /** \brief Verify that our objective is satisfied already and we can stop planning */
            virtual bool isSatisfied(Cost c) const
            {
                return this->isCostBetterThan(c, threshold_);
            }

            Cost getCostThreshold(void) const
            {
                return threshold_;
            }

            void setCostThreshold(Cost c)
            {
                threshold_ = c;
            }

            /** \brief Get the cost that corresponds to an entire path. This implementation assumes \e Path is of type \e PathGeometric.*/
            virtual Cost getCost(const Path &path) const;

	    /** \brief Check whether the the cost \e c1 is considered less than the cost \e c2. */
	    virtual bool isCostBetterThan(Cost c1, Cost c2) const
            {
                return c1.v < c2.v;
            }
	 
	    /** \brief Evaluate a cost map defined on the state space at a state \e s. Default implementation maps all states to 1.0. */
	    virtual Cost stateCost(const State *s) const
            {
                return Cost(1.0);
            }

	    /** \brief Get the cost that corresponds to the motion segment between \e s1 and \e s2 */
            virtual Cost motionCost(const State *s1, const State *s2) const = 0;

            /** \brief Get the cost that corresponds to combining the costs \e c1 and \e c2. Implementations of this method should allow for \e c1 and \e cost to point to the same memory location. */
            virtual Cost combineCosts(Cost c1, Cost c2) const
            {
                return Cost(c1.v + c2.v);
            }

	    /** \brief Get the cost corresponding to the beginning of a path that starts at \e s. */
	    // virtual void getInitialCost(const State *s, Cost *cost) const = 0;

            virtual Cost identityCost() const
            {
                return Cost(0.0);
            }

	    /** \brief Get a cost which is greater than all other costs in this OptimizationObjective; required for use in Dijkstra/Astar. */
	    virtual Cost infiniteCost() const
            {
                return Cost(std::numeric_limits<double>::infinity());
            }            

            virtual Cost initialCost(const State *s) const
            {
                return identityCost();
            }

            virtual Cost terminalCost(const State *s) const
            {
                return identityCost();
            }

	    /** \brief Check if this objective has a symmetric cost metric, i.e. motionCost(s1, s2) = motionCost(s2, s1). Default implementation returns whether the underlying state space has symmetric interpolation. */
	    virtual bool isSymmetric(void) const
            {
                return si_->getStateSpace()->hasSymmetricInterpolate();
            }

	    /** \brief Compute the average state cost of this objective by taking a sample of \e numStates states */
	    virtual Cost averageStateCost(unsigned int numStates) const;

            void setCostToGoHeuristic(CostToGoHeuristic costToGo)
            {
                costToGoFn_ = costToGo;
            }

            Cost costToGo(const State* state, const Goal* goal) const
            {
                if (costToGoFn_)
                    return costToGoFn_(state, goal);
                else
                    return this->identityCost(); // assumes that identity < all costs
            }

            // \TODO should this default to motionCost()?
            Cost motionCostHeuristic(const State* s1, const State* s2) const
            {
                return this->identityCost(); // assumes that identity < all costs
            }

            // Needed for operators in MultiOptimizationObjective
            const SpaceInformationPtr& getSpaceInformation(void) const;

        protected:
            /** \brief The space information for this objective */
            SpaceInformationPtr si_;

            /** \brief The description of this optimization objective */
            std::string description_;

            Cost threshold_;

            CostToGoHeuristic costToGoFn_;
        };

        class PathLengthOptimizationObjective : public OptimizationObjective
        {
        public:
            PathLengthOptimizationObjective(const SpaceInformationPtr &si) :
                OptimizationObjective(si) { description_ = "Path Length"; }

            virtual Cost motionCost(const State *s1, const State *s2) const
            {
                return Cost(si_->distance(s1, s2));
            }

            virtual Cost motionCostHeuristic(const State *s1, const State *s2) const
            {
                return motionCost(s1, s2);
            }
        };

	class StateCostIntegralObjective : public OptimizationObjective
	{
	public:	
	    StateCostIntegralObjective(const SpaceInformationPtr &si) :
                OptimizationObjective(si) { description_ = "State Cost Integral"; }

	    /** \brief Compute the cost of a path segment from \e s1 to \e s2 (including endpoints)
		\param s1 start state of the motion to be evaluated
		\param s2 final state of the motion to be evaluated
		\param cost the cost of the motion segment

		By default, this function computes
		\f{eqnarray*}{
		\mbox{cost} &=& \frac{cost(s_1) + cost(s_2)}{2}\vert s_1 - s_2 \vert
		\f}
	    */
	    virtual Cost motionCost(const State *s1, const State *s2) const;
	};

	class MechanicalWorkOptimizationObjective : public OptimizationObjective
	{
	public:
	    MechanicalWorkOptimizationObjective(const SpaceInformationPtr &si,
                                                double pathLengthWeight = 0.00001) :
		OptimizationObjective(si), 
		pathLengthWeight_(pathLengthWeight) {}

	    virtual double getPathLengthWeight(void) const;
	    virtual void setPathLengthWeight(double weight);

	    virtual Cost motionCost(const State *s1, const State *s2) const;

	protected:
	    double pathLengthWeight_;
	};

        // The cost of a path is defined as the worst state cost over
        // the entire path. This objective attempts to find the path
        // with the "best worst cost" over all paths.
        class MinimaxObjective : public OptimizationObjective
        {
        public:
            MinimaxObjective(const SpaceInformationPtr &si) :
                OptimizationObjective(si) {}

            // assumes all costs are worse than identity
            virtual Cost motionCost(const State *s1, const State *s2) const;
            virtual Cost combineCosts(Cost c1, Cost c2) const;
        };

        class MaximizeMinClearanceObjective : public MinimaxObjective
        {
        public:
            MaximizeMinClearanceObjective(const SpaceInformationPtr &si) :
                MinimaxObjective(si) 
            { 
                this->setCostThreshold(Cost(std::numeric_limits<double>::infinity()));
            }

            virtual Cost stateCost(const State* s) const;
            virtual bool isCostBetterThan(Cost c1, Cost c2) const;
            virtual Cost identityCost(void) const;
            virtual Cost infiniteCost(void) const;
        };

        // When goal region's distanceGoal() is equivalent to the
        // cost-to-go of a state under the optimization objective
        struct GoalRegionDistanceCostToGo
        {
            Cost operator()(const State* state, const Goal* goal) const;
        };

        class MultiOptimizationObjective : public OptimizationObjective
        {
        public:
            MultiOptimizationObjective(const SpaceInformationPtr &si) :
                OptimizationObjective(si),
                locked_(false) {}

            void addObjective(const OptimizationObjectivePtr& objective, 
                              double weight = 1.0);

            std::size_t getObjectiveCount(void) const;

            const OptimizationObjectivePtr& getObjective(unsigned int idx) const;

            double getObjectiveWeight(unsigned int idx) const;
            void setObjectiveWeight(unsigned int idx, double weight);

            void lock(void);
            bool isLocked(void) const;

            // For now, assumes we simply use addition to add up all
            // objective cost values, where each individual value is
            // scaled by its weight
            virtual Cost stateCost(const State* s) const;

            // Same as stateCost
            virtual Cost motionCost(const State* s1, const State* s2) const;

        protected:

            struct Component
            {
                Component(const OptimizationObjectivePtr& obj, double weight) :
                    objective(obj), weight(weight) {}
                OptimizationObjectivePtr objective;
                double weight;
            };

            std::vector<Component> components_;
            bool locked_;

            friend OptimizationObjectivePtr operator+(const OptimizationObjectivePtr &a,
                                                      const OptimizationObjectivePtr &b);

            friend OptimizationObjectivePtr operator*(double w, const OptimizationObjectivePtr &a);

            friend OptimizationObjectivePtr operator*(const OptimizationObjectivePtr &a, double w);
        };

        // For now, assume that the SpaceInformation to be associated
        // with the new objective is that of a
        OptimizationObjectivePtr operator+(const OptimizationObjectivePtr &a,
                                           const OptimizationObjectivePtr &b);

        OptimizationObjectivePtr operator*(double w, const OptimizationObjectivePtr &a);
      
        OptimizationObjectivePtr operator*(const OptimizationObjectivePtr &a, double w);
    }
}

#endif
