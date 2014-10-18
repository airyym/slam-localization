#define BOOST_TEST_MODULE template_for_test_test
#include <boost/test/included/unit_test.hpp>

/** Library **/
#include <localization/filters/UsckfError.hpp> /** USCKF class with Manifolds */
#include <localization/filters/MtkWrap.hpp> /** USCKF wrapper for the state vector */
#include <localization/filters/State.hpp> /** Filters State */
#include <localization/Configuration.hpp> /** Constant values of the library */

/** Eigen **/
#include <Eigen/Core> /** Core */
#include <Eigen/StdVector> /** For STL container with Eigen types **/

/** Standard libs **/
#include <iostream>
#include <vector>

std::vector<int> hola(100, 2);
typedef std::vector< MTK::vect<3, double> > MTKintFeature;

MTKintFeature adios(100, Eigen::Vector3d::Zero() );

/** Wrap the AugmentedState **/
typedef localization::MtkWrap<localization::AugmentedState> WAugmentedState;
typedef localization::MtkWrap<localization::State> WSingleState;

//acc is acceleration without any perturbation
//angvelo is angular velocity (gyros) without any perturbation
WSingleState process_model (const WSingleState &serror, const Eigen::Matrix<double, 3, 1> &acc,
                        const Eigen::Matrix<double, 3, 1> &angvelo,
                        const Eigen::Quaternion<double> &orientq, double dt)
{
    WSingleState s2;
    Eigen::Matrix <double,3,3> dFki; /** Discrete form of subsytem models */

    //std::cout<<"[PROCESS_MODEL] acc:\n"<<acc<<"\n";
    //std::cout<<"[PROCESS_MODEL] angevelo:\n"<<angvelo<<"\n";

    /** Propagate the position (discretization approx) **/
    dFki = Eigen::Matrix<double, 3, 3>::Identity() * dt +
        0.5 * Eigen::Matrix<double, 3, 3>::Identity() * Eigen::Matrix<double, 3, 3>::Identity() * pow(dt,2);
    s2.pos = serror.pos + dFki * serror.velo;

    /** Propagate the velocity (discretization approx) **/
    Eigen::Matrix <double,3, 1> deltaVel;
    deltaVel = /*orientq.inverse() */ (acc * dt); /** TO-DO: Increment in velocity in world frame */
    s2.velo = serror.velo +  serror.orient * deltaVel;

    /** Propagate the error quaternion **/
    Eigen::Matrix <double,3, 1> deltaAngle = angvelo * dt;
    s2.orient = serror.orient;
    s2.orient.boxplus(deltaAngle);

    return s2;
}

localization::Usckf <WAugmentedState, WSingleState>::SingleStateCovariance processNoiseCov (double dt)
{
    localization::Usckf <WAugmentedState, WSingleState>::SingleStateCovariance cov =
        localization::Usckf <WAugmentedState, WSingleState>::SingleStateCovariance::Zero();
    MTK::setDiagonal (cov, &WSingleState::pos, 0.1 * dt);
    MTK::setDiagonal (cov, &WSingleState::orient, 0.1 * dt);
    MTK::setDiagonal (cov, &WSingleState::velo,  0.1 * dt);
    MTK::setDiagonal (cov, &WSingleState::angvelo,  0.1 * dt);

    return cov ;
}

BOOST_AUTO_TEST_CASE( USCKF )
{

    localization::vecDynamic featuresVO, featuresICP;
    featuresVO.resize(4,1);
    featuresVO<<3.34, 3.34, 3.34, 3.34;
    featuresICP.resize(10,1);
    featuresICP<<1.34, 1.34, 1.34, 1.34,1.34, 1.34, 1.34, 1.34, 1.34, 1.34;

    WAugmentedState vstate;
    WAugmentedState verror;

    std::cout<<"vstate::DOF is "<<vstate.DOF<<"\n";
    std::cout<<"vstate.statek is "<<vstate.statek<<"\n";
    std::cout<<"vstate.statek_l is "<<vstate.statek_l<<"\n";
    std::cout<<"vstate.statek_i is "<<vstate.statek_i<<"\n";
    std::cout<<"vstate: "<<vstate<<"\n";
    std::cout<<"vectorized state:\n"<<vstate.getVectorizedState()<<"\n";
    std::cout<<"featuresICP.size():\n"<<featuresICP.size() <<"\n";
    std::cout<<"featuresICP:\n"<<featuresICP.matrix()<<"\n";

    //std::vector< Eigen::Matrix <double, 3, 1> , Eigen::aligned_allocator < Eigen::Matrix <double, 3, 1> > > featuresVO;

    std::cout<<"size of featuresk: "<<vstate.featuresk.size()<<"\n";
    vstate.featuresk = featuresVO;
    std::cout<<"size of featuresk: "<<vstate.featuresk.size()<<"\n";
    vstate.featuresk_l = featuresICP;
    std::cout<<"size of featuresk_l: "<<vstate.featuresk_l.size()<<"\n";
    std::cout<<"vstate: "<<vstate<<"\n";
    std::cout<<"vectorized state:\n"<<vstate.getVectorizedState()<<"\n";

    localization::AugmentedState vstatebis(vstate.statek, vstate.statek_l, vstate.statek_i, featuresVO, featuresICP);
    std::cout<<"vstatebis::DOF is "<<vstatebis.getDOF()<<"\n";

    const double dt = 0.01; /** 100Hz */

    /** Initial covariance matrix **/
    localization::Usckf<WAugmentedState, WSingleState>::AugmentedStateCovariance P0;
    MTK::subblock (P0, &WAugmentedState::statek, &WAugmentedState::statek) = 0.0025 * localization::Usckf<WAugmentedState, WSingleState>::SingleStateCovariance::Identity();
    MTK::subblock (P0, &WAugmentedState::statek_l, &WAugmentedState::statek_l) = 0.0035 * localization::Usckf<WAugmentedState, WSingleState>::SingleStateCovariance::Identity();
    MTK::subblock (P0, &WAugmentedState::statek_i, &WAugmentedState::statek_i) = 0.0045 * localization::Usckf<WAugmentedState, WSingleState>::SingleStateCovariance::Identity();

    MTK::subblock (P0, &WAugmentedState::statek, &WAugmentedState::statek_l) = 0.0011 * localization::Usckf<WAugmentedState, WSingleState>::SingleStateCovariance::Identity();
    MTK::subblock (P0, &WAugmentedState::statek, &WAugmentedState::statek_i) = 0.0012 * localization::Usckf<WAugmentedState, WSingleState>::SingleStateCovariance::Identity();
    MTK::subblock (P0, &WAugmentedState::statek_l, &WAugmentedState::statek) = MTK::subblock (P0, &WAugmentedState::statek, &WAugmentedState::statek_l);
    MTK::subblock (P0, &WAugmentedState::statek_i, &WAugmentedState::statek) = MTK::subblock (P0, &WAugmentedState::statek, &WAugmentedState::statek_i);

    MTK::subblock (P0, &WAugmentedState::statek_l, &WAugmentedState::statek_i) = 0.0021 * localization::Usckf<WAugmentedState, WSingleState>::SingleStateCovariance::Identity();
    MTK::subblock (P0, &WAugmentedState::statek_i, &WAugmentedState::statek_l) = MTK::subblock (P0, &WAugmentedState::statek_l, &WAugmentedState::statek_i);

    //std::cout<<"[INITIALIZATION] P0:\n"<<P0<<std::endl;

    localization::Usckf<WAugmentedState, WSingleState> filter(static_cast<const WAugmentedState> (vstate),
                                                        static_cast<const WAugmentedState> (verror),
                                                        static_cast<const localization::Usckf<WAugmentedState,
                                                        WSingleState>::AugmentedStateCovariance> (P0));
    vstate = filter.muError();
    Eigen::Matrix <double,localization::NUMAXIS,1> eulerinit; /** In Euler angles **/
    eulerinit[2] = vstate.statek_i.orient.toRotationMatrix().eulerAngles(2,1,0)[0];//Yaw
    eulerinit[1] = vstate.statek_i.orient.toRotationMatrix().eulerAngles(2,1,0)[1];//Pitch
    eulerinit[0] = vstate.statek_i.orient.toRotationMatrix().eulerAngles(2,1,0)[2];//Roll
    std::cout<<"Init Roll: "<<eulerinit[0]*localization::R2D<<" Pitch: "<<eulerinit[1]*localization::R2D<<" Yaw: "<<eulerinit[2]*localization::R2D<<"\n";

    for (register int i=0; i<1; ++i)
    {
        Eigen::Vector3d acc, gyro;
        Eigen::Quaternion<double> orientq = filter.muState().statek_i.orient;
        acc << 100.00, 0.00, 0.00;
        gyro << (1000.00*localization::D2R), (1000.00*localization::D2R), (1000.00*localization::D2R);
        std::cout<<"IN_LOOP ["<<i<<"]\n";
        localization::Usckf <WAugmentedState, WSingleState>::SingleStateCovariance myCov = processNoiseCov(dt);
        filter.predict(boost::bind(process_model, _1 , acc , gyro, orientq, dt), myCov);
        //std::cout<<"IN_LOOP Pk+i|k+l|k\n"<<filter.PkAugmentedState();
    }

    vstate = filter.muError();
    Eigen::Matrix <double,localization::NUMAXIS,1> euler; /** In euler angles **/
    euler[2] = vstate.statek_i.orient.toRotationMatrix().eulerAngles(2,1,0)[0];//Yaw
    euler[1] = vstate.statek_i.orient.toRotationMatrix().eulerAngles(2,1,0)[1];//Pitch
    euler[0] = vstate.statek_i.orient.toRotationMatrix().eulerAngles(2,1,0)[2];//Roll
    std::cout<<"Result Roll: "<<euler[0]*localization::R2D<<" Pitch: "<<euler[1]*localization::R2D<<" Yaw: "<<euler[2]*localization::R2D<<"\n";

    Eigen::AngleAxisd angleAxis (vstate.statek_i.orient);
    std::cout<<"The angle of rotation is: "<<angleAxis.angle()<<"\n";
    std::cout<<"The angle of rotation is: "<<(angleAxis.axis() * angleAxis.angle()).norm()<<"\n";
    std::cout<<"The angle of rotation is (degrees): "<<angleAxis.angle()*localization::R2D<<"\n";
    std::cout<<"The axis of rotation is:\n"<<angleAxis.axis()<<"\n";

    euler[2] = 0.00 * localization::D2R;
    euler[1] = 0.00 * localization::D2R;
    euler[0] = 0.30 * localization::D2R;

    vstate.statek_i.orient.boxplus(euler);
    euler[2] = vstate.statek_i.orient.toRotationMatrix().eulerAngles(2,1,0)[0];//Yaw
    euler[1] = vstate.statek_i.orient.toRotationMatrix().eulerAngles(2,1,0)[1];//Pitch
    euler[0] = vstate.statek_i.orient.toRotationMatrix().eulerAngles(2,1,0)[2];//Roll
    std::cout<<"Result(Euler) Roll: "<<euler[0]*localization::R2D<<" Pitch: "<<euler[1]*localization::R2D<<" Yaw: "<<euler[2]*localization::R2D<<"\n";

    euler[2] = 0.05 * localization::D2R;
    euler[1] = 0.05 * localization::D2R;
    euler[0] = 0.05 * localization::D2R;

    vstate.statek_i.orient.boxplus(euler);
    euler[2] = vstate.statek_i.orient.toRotationMatrix().eulerAngles(2,1,0)[0];//Yaw
    euler[1] = vstate.statek_i.orient.toRotationMatrix().eulerAngles(2,1,0)[1];//Pitch
    euler[0] = vstate.statek_i.orient.toRotationMatrix().eulerAngles(2,1,0)[2];//Roll
    std::cout<<"Result(Euler2) Roll: "<<euler[0]*localization::R2D<<" Pitch: "<<euler[1]*localization::R2D<<" Yaw: "<<euler[2]*localization::R2D<<"\n";

    std::cout<<"vectorized state: "<<vstate.getVectorizedState()<<"\n";


    //std::cout<<"vstate.featuresk is "<<vstate.featuresk<<"\n";
    //std::cout<<"vstate.featuresk_l is "<<vstate.featuresk_l<<"\n";




}
