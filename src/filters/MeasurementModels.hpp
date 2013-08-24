#ifndef _MEASUREMENT_MODELS_HPP_
#define _MEASUREMENT_MODELS_HPP_

#include <Eigen/StdVector> /** For STL container with Eigen types **/
#include <Eigen/LU> /** Lineal algebra of Eigen */
#include <Eigen/SVD> /** Singular Value Decomposition (SVD) of Eigen */

#define MEASUREMENT_MODEL_DEBUG_PRINTS 1

namespace localization
{
    /**@brief Measurement Model for the attitude and velocity correction
     */
    template <int _SingleStateDoF>
    Eigen::Matrix < double, 6, _SingleStateDoF > proprioceptiveMeasurementMatrix
                    (const Eigen::Quaterniond &orient, const double gravity)
    {
        Eigen::Matrix < double, 6, _SingleStateDoF > H; /** Measurement matrix of the model */
        Eigen::Vector3d gtilde; /** Gravitation in the world frame */
        Eigen::Vector3d gtilde_body; /** Gravitation in the body frame */
        Eigen::Matrix3d gtilde2product; /** Vec 2 product  matrix for the gravity vector in body frame */

        /** Init and gtilde **/
        H.setZero();
        gtilde <<0.00, 0.00, gravity;

        /** Form the matrix for the measurement of the velocity correction **/
        H.template block<3,3>(0,3).setIdentity();

        /** Calculate the gravity vector in the body frame **/
        gtilde_body = orient.inverse() * gtilde;

        /** Cross matrix of the g vector in body frame **/
        gtilde2product << 0, -gtilde_body(2), gtilde_body(1),
		    gtilde_body(2), 0, -gtilde_body(0),
    		    -gtilde_body(1), gtilde_body(0), 0;

        /** Form the matrix for the measurement of the attitude (acc correction) **/
        H.template block<3,3>(3,6) = 2.0*gtilde2product;
        H(3,_SingleStateDoF-3) = 1; H(4,_SingleStateDoF-2) = 1; H(5,_SingleStateDoF-1) = 1;

        /*#ifdef MEASUREMENT_MODEL_DEBUG_PRINTS
        std::cout<<"[MEASUREMENT_MATRIX] H is of size "<< H.rows() <<" x "<<H.cols()<<"\n";
        std::cout<<"[MEASUREMENT_MATRIX] H:\n"<< H <<"\n";
        #endif*/

        return H;
    }

    template <typename _SingleState, typename _VectorizedType>
    _VectorizedType getVectorizedSingleState (const _SingleState & state)
    {
        _VectorizedType vstate;

        vstate.template block<localization::vec3::DOF, 1>(0,0) = state.pos; //!Position
        vstate.template block<localization::vec3::DOF, 1>(localization::vec3::DOF,0) = state.vel; //!Velocity

        Eigen::Matrix<double, localization::SO3::DOF, 1> qe; //! Error quaternion
        qe << state.orient.x(), state.orient.y(), state.orient.z();
        vstate.template block<localization::SO3::DOF, 1>(2*localization::vec3::DOF, 0) = qe;

        vstate.template block<localization::vec3::DOF, 1>(2*localization::vec3::DOF + localization::SO3::DOF, 0) = state.gbias; //! Gyros bias
        vstate.template block<localization::vec3::DOF, 1>(3*localization::vec3::DOF + localization::SO3::DOF, 0) = state.abias; //! Acc bias

        return vstate;
    }


    template <typename _SingleState, typename _MeasurementVector, typename _VectorizedType >
        _MeasurementVector proprioceptiveMeasurementModel (const _SingleState &statek_i, const Eigen::Matrix<double, 6, _SingleState::DOF> &H)
    {
        _MeasurementVector z_model;

        _VectorizedType xk_i = getVectorizedSingleState<_SingleState, _VectorizedType> (statek_i);

        z_model = H * xk_i;

        /*#ifdef MEASUREMENT_MODEL_DEBUG_PRINTS
        std::cout<<"[MEASUREMENT_MODEL] z_model:\n"<< z_model<<"\n";
        std::cout<<"[MEASUREMENT_MODEL] H is of size "<< H.rows() <<" x "<<H.cols()<<"\n";
        std::cout<<"[MEASUREMENT_MODEL] H:\n"<< H <<"\n";
        #endif*/


        return z_model;
    }

    /**@brief Noise Measurement Matrix for the attitude and velocity correction
     */
    Eigen::Matrix<double, 6, 6> proprioceptiveMeasurementNoiseCov (const base::Vector3d &accrw, double &delta_t)
    {
        Eigen::Matrix<double, 6, 6> Cov; /** Cov. matrix of the measurement **/
        Eigen::Matrix3d Rat; /** Gravity vector covariance matrix */
        Cov.setZero(); Rat.setZero();

        double sqrtdelta_t = sqrt(delta_t); /** Square root of delta time interval */

        /** Part of the gravity **/
        Rat(0,0) = 3 * pow(accrw[0]/sqrtdelta_t,2);//0.0054785914701378034;
        Rat(1,1) = 3 * pow(accrw[1]/sqrtdelta_t,2);//0.0061094546837916494;
        Rat(2,2) = 3 * pow(accrw[2]/sqrtdelta_t,2);//0.0063186020143245212;

        /** To-DO: Part for the velocity **/
        Cov.block<3,3> (3,3) = Rat;

        #ifdef MEASUREMENT_MODEL_DEBUG_PRINTS
        std::cout<<"[MEASUREMENT_MODEL] Cov is of size "<< Cov.rows() <<" x "<<Cov.cols()<<"\n";
        std::cout<<"[MEASUREMENT_MODEL] Cov:\n"<< Cov <<"\n";
        #endif

        return Cov;
    }

    /**@brief Class for Adaptive measurement matrix for the attitude correction in 3D
     */
    class AdaptiveAttitudeCov
    {

    protected:

        unsigned int r1count;/** Variable used in the adaptive algorithm, to compute the Uk matrix for SVD*/
        unsigned int m1; /** Parameter for adaptive algorithm (to estimate Uk with is not directly observale) */
        unsigned int m2; /** Parameter for adaptive algorithm (to prevent falsering entering in no-external acc mode) */
        double gamma; /** Parameter for adaptive algorithm (only entering when Qstart is greater than RHR'+Ra) */
        unsigned int r2count; /** Parameter for adaptive algorithm */

        /** History of M1 measurement noise convariance matrix (for the adaptive algorithm) */
        std::vector < Eigen::Matrix3d, Eigen::aligned_allocator < Eigen::Matrix3d > > RHist;

    public:

        AdaptiveAttitudeCov(const unsigned int M1, const unsigned int M2,
                        const double GAMMA, const unsigned int R2COUNT)
            :m1(M1), m2(M2), gamma(GAMMA), r2count(R2COUNT)
        {
            r1count = 0;
            RHist.resize(M1);
            for (std::vector< Eigen::Matrix3d, Eigen::aligned_allocator < Eigen::Matrix3d > >::iterator it = RHist.begin()
                    ; it != RHist.end(); ++it)
                (*it).setZero();

            #ifdef MEASUREMENT_MODEL_DEBUG_PRINTS
            std::cout<<"[INIT ADAPTIVE_ATTITUDE] M1: "<<m1<<"\n";
            std::cout<<"[INIT ADAPTIVE_ATTITUDE] M2: "<<m2<<"\n";
            std::cout<<"[INIT ADAPTIVE_ATTITUDE] GAMMA: "<<gamma<<"\n";
            std::cout<<"[INIT ADAPTIVE_ATTITUDE] r1count: "<<r1count<<"\n";
            std::cout<<"[INIT ADAPTIVE_ATTITUDE] r2count: "<<r2count<<"\n";
            std::cout<<"[INIT ADAPTIVE_ATTITUDE] RHist is of size "<<RHist.size()<<"\n";
            #endif

        }

        ~AdaptiveAttitudeCov(){}

        template <int _DoFState>
        Eigen::Matrix3d matrix(const Eigen::Matrix <double, _DoFState, 1> xk,
                            const Eigen::Matrix <double, _DoFState, _DoFState> &Pk,
                            const Eigen::Vector3d &z,
                            const Eigen::Matrix <double, 3, _DoFState> &H,
                            const Eigen::Matrix3d &R)
        {
            Eigen::Matrix3d R1a; /** Measurement noise convariance matrix for the adaptive algorithm */
            Eigen::Matrix3d fooR; /**  Measurement noise matrix from accelerometers matrix Ra */
            Eigen::Matrix3d Uk; /** Uk measurement noise convariance matrix for the adaptive algorithm */
            Eigen::Matrix3d Qstar; /** External acceleration covariance matrix */
            Eigen::Matrix3d u; /** Unitary matrix U for the SVD decomposition */
            Eigen::Vector3d lambda; /** Lambda vector for the adaptive algorithm */
            Eigen::Vector3d mu; /** mu vector for the adaptive algorithm */
            Eigen::Vector3d s; /** Unitary matrix V for the SVD decomposition */

            /** Estimation of R **/
            R1a = (z - H*xk) * (z - H*xk).transpose();

            RHist[r1count] = R1a;

            #ifdef MEASUREMENT_MODEL_DEBUG_PRINTS
            std::cout<<"[ADAPTIVE_ATTITUDE] xk:\n"<<xk<<"\n";
            std::cout<<"[ADAPTIVE_ATTITUDE] Pk:\n"<<Pk<<"\n";
            std::cout<<"[ADAPTIVE_ATTITUDE] z:\n"<<z<<"\n";
            std::cout<<"[ADAPTIVE_ATTITUDE] H:\n"<<H<<"\n";
            std::cout<<"[ADAPTIVE_ATTITUDE] R:\n"<<R<<"\n";
            std::cout<<"[ADAPTIVE_ATTITUDE] r1count:\n"<<r1count<<"\n";
            std::cout<<"[ADAPTIVE_ATTITUDE] R1a:\n"<<R1a<<"\n";
            std::cout<<"[ADAPTIVE_ATTITUDE] z:\n"<<z<<"\n";
            #endif


            /** r1count + 1 modulus the number of history M1 **/
            r1count = (r1count+1)%(m1);

            Uk.setZero();
            /** Starting the Uk is R **/
            for (register int j=0; j<static_cast<int>(m1); j++)
            {
                Uk += RHist[j];
            }

            Uk = Uk/static_cast<double>(m1);

            fooR = H*Pk*H.transpose() + R;

            /**
            * Single Value Decomposition
            */
            Eigen::JacobiSVD <Eigen::MatrixXd > svdOfUk(Uk, Eigen::ComputeThinU);

            s = svdOfUk.singularValues(); //!eigenvalues
            u = svdOfUk.matrixU();//!eigenvectors

            lambda << s(0), s(1), s(2);

            mu(0) = u.col(0).transpose() * fooR * u.col(0);
            mu(1) = u.col(1).transpose() * fooR * u.col(1);
            mu(2) = u.col(2).transpose() * fooR * u.col(2);

            #ifdef MEASUREMENT_MODEL_DEBUG_PRINTS
            std::cout<<"[ADAPTIVE_ATTITUDE] (lambda - mu) is:\n"<<(lambda - mu)<<"\n";
            #endif

            if ((lambda - mu).maxCoeff() > gamma)
            {

                #ifdef ADAPTIVE_ATTITUDE_DEBUG_PRINTS
                std::cout<<"[ADAPTIVE_ATTITUDE] Bigger than GAMMA("<<gamma<<")\n";
                #endif

                r2count = 0;
                Eigen::Vector3d auxvector; /** Auxiliar vector variable */
                auxvector(0) = std::max(lambda(0)-mu(0),static_cast<double>(0.00));
                auxvector(1) = std::max(lambda(1)-mu(1),static_cast<double>(0.00));
                auxvector(2) = std::max(lambda(2)-mu(2),static_cast<double>(0.00));

                Qstar = auxvector(0) * u.col(0) * u.col(0).transpose() + auxvector(1) * u.col(1) * u.col(1).transpose() + auxvector(2) * u.col(2) * u.col(2).transpose();
            }
            else
            {
                #ifdef ADAPTIVE_ATTITUDE_DEBUG_PRINTS
                std::cout<<"[ADAPTIVE_ATTITUDE] Lower than GAMMA("<<gamma<<") r2count: "<<r2count<<"\n";
                #endif

                r2count ++;
                if (r2count < m2)
                {
                    Eigen::Vector3d auxvector; /** Auxiliar vector variable */
                    auxvector(0) = std::max(lambda(0)-mu(0),static_cast<double>(0.00));
                    auxvector(1) = std::max(lambda(1)-mu(1),static_cast<double>(0.00));
                    auxvector(2) = std::max(lambda(2)-mu(2),static_cast<double>(0.00));

                    Qstar = auxvector(0) * u.col(0) * u.col(0).transpose() + auxvector(1) * u.col(1) * u.col(1).transpose() + auxvector(2) * u.col(2) * u.col(2).transpose();
                }
                else
                    Qstar = Eigen::Matrix3d::Zero();
            }

            return Qstar;
        }
    };
}
#endif /** End of MeasurementModels.hpp **/
