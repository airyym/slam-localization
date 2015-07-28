#define BOOST_TEST_MODULE template_for_test_test
#include <boost/test/included/unit_test.hpp>
#include <boost/shared_ptr.hpp> /** For shared pointers **/


/** Library **/
#include <localization/filters/Msckf.hpp> /** MSCKF_DYNAMIC class with Manifolds */
#include <localization/filters/MtkWrap.hpp> /** USCKF_DYNAMIC wrapper for the state vector */
#include <localization/filters/State.hpp> /** Filters State */
#include <localization/Configuration.hpp> /** Constant values of the library */

/** Rock Types **/
#include <base/Eigen.hpp>
#include <base/samples/RigidBodyState.hpp>

/** Eigen **/
#include <Eigen/Core> /** Core */
#include <Eigen/StdVector> /** For STL container with Eigen types **/

/** Standard libs **/
#include <iostream>
#include <vector>

/** Wrap the Multi State **/
typedef localization::MtkWrap<localization::State> WSingleState;
typedef localization::MtkDynamicWrap< localization::MultiState<localization::SensorState> > WMultiState;
typedef localization::Msckf<WMultiState, WSingleState> MultiStateFilter;
typedef ::MTK::vect<Eigen::Dynamic, double> MeasurementType;



/** Process model when accumulating delta poses **/
WSingleState processModel (const WSingleState &state,  const Eigen::Vector3d &delta_position, const localization::SO3 &delta_orientation,
                            const Eigen::Vector3d &velocity, const Eigen::Vector3d &angular_velocity)
{
    WSingleState s2; /** Propagated state */

    /** Apply Rotation **/
    s2.orient = state.orient * delta_orientation;
    s2.angvelo = angular_velocity;

    /** Apply Translation **/
    s2.pos = state.pos + (s2.orient * delta_position);
    s2.velo = velocity;

    return s2;
};


BOOST_AUTO_TEST_CASE( STATES )
{
    WMultiState mstate;

    BOOST_TEST_MESSAGE("[MULTI STATE] mstate::DOF is "<<mstate.DOF);
    BOOST_TEST_MESSAGE("[MULTI STATE] mstate.getDOF() is "<<mstate.getDOF());
    BOOST_TEST_MESSAGE("[MULTI STATE] mstate::SENSOR_DOF is "<<mstate.SENSOR_DOF);
    BOOST_TEST_MESSAGE("[MULTI STATE] mstate.statek is "<<mstate.statek);
    BOOST_TEST_MESSAGE("[MULTI STATE] mstate: "<<mstate);
    BOOST_TEST_MESSAGE("[MULTI STATE] vectorized state:\n"<<mstate.getVectorizedState());
    BOOST_TEST_MESSAGE("[MULTI STATE] vectorized state size :\n"<<mstate.getVectorizedState().size());
    BOOST_CHECK(mstate.getDOF() == mstate.getVectorizedState().size());
    BOOST_CHECK(mstate == mstate);

    WMultiState mstatebis;
    mstatebis.set(mstate.getVectorizedState());
    BOOST_CHECK(mstate == mstatebis);

}

BOOST_AUTO_TEST_CASE( OPERATIONS )
{
    WMultiState mstate, mstatebis;
    mstatebis.statek.pos<< 1, 2.0, -3.00;


    Eigen::Vector3d euler; /** In euler angles **/
    euler[2] = 1.00 * localization::D2R;
    euler[1] = 1.00 * localization::D2R;
    euler[0] = 1.00 * localization::D2R;

    mstatebis.statek.orient.boxplus(euler);

    localization::SensorState sstate(mstatebis.statek.pos, mstatebis.statek.orient);
    for (std::vector<localization::SensorState>::iterator it = mstatebis.sensorsk.begin();
                    it != mstatebis.sensorsk.end(); ++it)
    {
        it->set(sstate.getVectorizedState());
    }

    BOOST_TEST_MESSAGE("[OPERATIONS] mstate\n"<< mstate  );
    BOOST_TEST_MESSAGE("[OPERATIONS] mstatebis\n"<< mstatebis  );
    /** Operation with states **/
    WMultiState sumstate, resstate;
    WMultiState::vectorized_type vresstate, deltastate;
    deltastate.resize(sumstate.getDOF(), 1);
    vresstate.resize(sumstate.getDOF(), 1);

    vresstate = mstate - mstatebis;
    deltastate = vresstate;
    resstate.set(vresstate);
    BOOST_TEST_MESSAGE("[OPERATIONS] mstate - mstatebis\n"<< mstate - mstatebis );
    BOOST_TEST_MESSAGE("[OPERATIONS] mstate + deltastate\n"<< mstate + deltastate );
    sumstate = mstate + deltastate;
    BOOST_CHECK(resstate == sumstate);
    deltastate = -vresstate;
    sumstate = mstate + deltastate;
    BOOST_CHECK(mstatebis == sumstate);

    BOOST_TEST_MESSAGE("[OPERATIONS] resstate\n"<< resstate );
    BOOST_TEST_MESSAGE("[OPERATIONS] sumstate\n"<< sumstate );

   // mstatebis = mstate;
   // BOOST_TEST_MESSAGE("[OPERATIONS] mstatebis = mstate\n"<< mstatebis );
}

BOOST_AUTO_TEST_CASE( MATRIX_OPERATIONS )
{

    typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> MultiStateCovariance;
    typedef std::vector<WMultiState> MultiStateSigma;

    typedef typename WMultiState::SingleState SingleState;
    typedef typename WMultiState::SingleState::vectorized_type VectorizedSingleState;
    typedef Eigen::Matrix<double, int(WMultiState::SingleState::DOF), int(WMultiState::SingleState::DOF)> SingleStateCovariance;
    typedef std::vector<WMultiState::SingleState> SingleStateSigma;

    const unsigned int number_sensor_poses = 4;

    MultiStateCovariance Pk;
    Pk.resize(WSingleState::DOF + WMultiState::SENSOR_DOF * number_sensor_poses, WSingleState::DOF + WMultiState::SENSOR_DOF * number_sensor_poses); Pk.setIdentity();
    Pk.diagonal().segment(0, WMultiState::SingleState::DOF).setConstant(4);
    Pk.diagonal().segment(WMultiState::SingleState::DOF, WMultiState::SENSOR_DOF * number_sensor_poses).setConstant(6);

    Eigen::Matrix<double, WMultiState::SingleState::DOF, Eigen::Dynamic> Pkk;
    Pkk.resize(WMultiState::SingleState::DOF, WMultiState::SENSOR_DOF * number_sensor_poses);

    Pkk = Pk.block(0, WMultiState::SingleState::DOF, WMultiState::SingleState::DOF, WMultiState::SENSOR_DOF*number_sensor_poses);
    Pkk = 3.5 * Eigen::MatrixXd::Ones(WMultiState::SingleState::DOF, WMultiState::SENSOR_DOF * number_sensor_poses);
    Pk.block(0, WMultiState::SingleState::DOF, WMultiState::SingleState::DOF, WMultiState::SENSOR_DOF*number_sensor_poses) = Pkk;
    Pk.block(WMultiState::SingleState::DOF, 0, WMultiState::SENSOR_DOF*number_sensor_poses, WMultiState::SingleState::DOF) = Pkk.transpose();
    BOOST_TEST_MESSAGE("Pk ["<<Pk.rows() <<" x "<<Pk.cols());
    BOOST_TEST_MESSAGE("Pk:\n"<<Pk);
}

BOOST_AUTO_TEST_CASE( MSCKF )
{
    typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> MultiStateCovariance;
    const unsigned int number_sensor_poses = 4;

    WMultiState statek_0;
    MultiStateCovariance Pk_0;
    Pk_0.resize(WSingleState::DOF + WMultiState::SENSOR_DOF * number_sensor_poses, WSingleState::DOF + WMultiState::SENSOR_DOF * number_sensor_poses);
    Pk_0 = 0.025 * Eigen::MatrixXd::Ones(WMultiState::SingleState::DOF, WMultiState::SENSOR_DOF * number_sensor_poses);
    Pk_0 = 0.025 * Pk_0.setIdentity();

    /** Delta pose to integrate **/
    base::samples::RigidBodyState delta_pose;
    delta_pose.position << 0.1, 0.1, 0.1;
    delta_pose.orientation = Eigen::Quaternion <double>(
                    Eigen::AngleAxisd(1.0 * localization::D2R, Eigen::Vector3d::UnitZ())*
                    Eigen::AngleAxisd(1.0 * localization::D2R, Eigen::Vector3d::UnitY()) *
                    Eigen::AngleAxisd(1.0 * localization::D2R, Eigen::Vector3d::UnitX()));
    delta_pose.velocity << 0.1, 0.1, 0.1;
    delta_pose.angular_velocity << 0.1, 0.1, 0.1;

    typedef MultiStateFilter::SingleStateCovariance SingleStateCovariance;
    SingleStateCovariance cov_process; cov_process.setZero();
    MTK::subblock (cov_process, &WSingleState::pos, &WSingleState::pos) = 0.01 * Eigen::Matrix3d::Identity();
    MTK::subblock (cov_process, &WSingleState::orient, &WSingleState::orient) =  0.01 * Eigen::Matrix3d::Identity();
    MTK::subblock (cov_process, &WSingleState::velo, &WSingleState::velo) =  0.01 * Eigen::Matrix3d::Identity();
    MTK::subblock (cov_process, &WSingleState::angvelo, &WSingleState::angvelo) =  0.01 * Eigen::Matrix3d::Identity();

    boost::shared_ptr<MultiStateFilter> filter;
    filter.reset(new MultiStateFilter(statek_0, Pk_0));

    BOOST_TEST_MESSAGE("[MSCKF] statek_0\n"<<filter->muState());
    BOOST_TEST_MESSAGE("[MSCKF] P0 is of size "<<filter->getPk().rows() <<" x "<<filter->getPk().cols());
    BOOST_TEST_MESSAGE("[MSCKF] P0\n"<<filter->getPk());
    BOOST_TEST_MESSAGE("[MSCKF] statek_0.statek\n"<<filter->muSingleState());
    BOOST_TEST_MESSAGE("[MSCKF] P0_statek is of size "<<filter->getPkSingleState().rows()<<" x "<<filter->getPkSingleState().cols());
    BOOST_TEST_MESSAGE("[MSCKF] P0_statek\n"<<filter->getPkSingleState());

    /***************************/
    /** PROPAGATION / PREDICT **/
    /***************************/
    BOOST_TEST_MESSAGE("***************");
    BOOST_TEST_MESSAGE("*** PREDICT ***");
    BOOST_TEST_MESSAGE("***************");

    for (register int i=0; i<2; ++i)
    {

        /** Predict the filter state **/
        filter->predict(boost::bind(processModel, _1 ,
                            static_cast<const Eigen::Vector3d>(delta_pose.position),
                            static_cast<const localization::SO3>(delta_pose.orientation),
                            static_cast<const Eigen::Vector3d>(delta_pose.velocity),
                            static_cast<const Eigen::Vector3d>(delta_pose.angular_velocity)),
                            cov_process);
    }

    /***************************/
    /** CORRECTION / UPDATE   **/
    /***************************/
    BOOST_TEST_MESSAGE("**************");
    BOOST_TEST_MESSAGE("*** UPDATE ***");
    BOOST_TEST_MESSAGE("**************");

    MeasurementType vo_features;
    std::vector<Eigen::Vector2d> vector_features(4, Eigen::Vector2d::Ones());
    vo_features.resize(2*vector_features.size(), 1);
    BOOST_TEST_MESSAGE("vector_features.size()\n"<<vector_features.size());
    BOOST_TEST_MESSAGE("vo_features.size()\n"<<vo_features.size());

    register size_t idx = 0;
    for (std::vector<Eigen::Vector2d>::iterator it = vector_features.begin();
            it != vector_features.end(); ++it)
    {
        *it << idx, idx;
        BOOST_TEST_MESSAGE("it:\n"<<*it);
        vo_features.block(idx * it->size(), 0, it->size(), 1) = *it;
        BOOST_TEST_MESSAGE("vo_features\n"<<vo_features.block(idx * it->size(), 0, it->size(), 1));
        idx++;
    }



}
