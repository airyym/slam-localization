/**\file Datamodel.hpp
 * Header function file and defines
 */

#ifndef _DATAMODEL_HPP_
#define _DATAMODEL_HPP_

#include <iostream>

#include <Eigen/Core> /** Core methods of Eigen implementation **/
#include <Eigen/Dense> /** for the algebra and transformation matrices **/
#include "Configuration.hpp" /** For the localization framework constant and configuration values **/

namespace localization	
{
    
    /** Class for representing a 3D slip, linear or contact angle velocity vector and
     * its uncertainty in the estimation by Weighted Least-Squares**/
    class DataModel
    {
    
    public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	Eigen::Matrix<double, Eigen::Dynamic, 1> data; //! instantaneous slip velocity vector
	Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> Cov; //! Covariance matrix

    public:
	
	DataModel();
	DataModel(const unsigned int dim);
	DataModel(Eigen::Matrix<double, Eigen::Dynamic, 1> &data, Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> &Cov);
	int size ();
	void fusion(const DataModel& data2);
	void safeFusion(const DataModel& data2);
	DataModel operator+(const DataModel& data2) const;
	DataModel operator-(const DataModel& data2) const;
	DataModel& operator=(const DataModel& dmodel);

    };
    
    /** Default std::cout function
     */
    std::ostream & operator<<(std::ostream &out, const  DataModel& trans);

}

#endif