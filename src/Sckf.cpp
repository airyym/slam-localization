/**\file Sckf.cpp
 *
 * This class has the primitive methods for an Stochastic Cloning Indirect Kalman Filter implementation
 * the state vector are formed by the errors. Therefore the name of indirect kalman filter 
 * 
 * 
 * @author Javier Hidalgo Carrio | DFKI RIC Bremen | javier.hidalgo_carrio@dfki.de
 * @date November 2012.
 * @version 1.0.
 */

#include "Sckf.hpp"

#define DEBUG_PRINTS 1

using namespace localization;
using namespace Eigen;

void Sckf::welcome()
{
	std::cout << "You successfully compiled and executed SCFK. Welcome!" << std::endl;
}


/**
* @brief Gets the current vector x
*/
Eigen::Matrix< double, Eigen::Dynamic, 1  > Sckf::getStatex()
{
    return xki_k;
}

/**
* @brief Gets the current orientation of the robot in Quaternion
*/
Eigen::Quaternion< double > Sckf::getAttitude()
{
    return this->q4;
}

/**
* @brief Gets the gravity value
*/
double Sckf::getGravity()
{
    return this->gtilde.norm();
}


/**
* @brief Gets the current orientation of the robot in Euler angles
*/
Eigen::Matrix< double, NUMAXIS , 1  > Sckf::getEuler()
{
    Eigen::Matrix <double, NUMAXIS, 1> euler;
    
    //std::cout << Eigen::Matrix3d(q4) << std::endl; 
    Eigen::Vector3d e = Eigen::Matrix3d(q4).eulerAngles(2,1,0);
    euler(0) = e[2]; 
    euler(1) = e[1]; 
    euler(2) = e[0]; 
    
    std::cout << "Attitude (getEuler): "<< euler(0)<<" "<<euler(1)<<" "<<euler(2)<<"\n";
    std::cout << "Attitude in degrees (getEuler): "<< euler(0)*R2D<<" "<<euler(1)*R2D<<" "<<euler(2)*R2D<<"\n";
    
    return euler;

}

/**
* @brief Gets Noise covariance matrix
*/
Eigen::Matrix< double, Eigen::Dynamic, Eigen::Dynamic > Sckf::getCovariancex()
{
    return Pki_k;
}

/**
* @brief Gets Noise covariance matrix for attitude estimation
*/
Eigen::Matrix< double, Sckf::A_STATE_VECTOR_SIZE, Sckf::A_STATE_VECTOR_SIZE > Sckf::getCovarianceAttitude()
{
    return Pki_k.block<Sckf::A_STATE_VECTOR_SIZE, Sckf::A_STATE_VECTOR_SIZE> (2*NUMAXIS, 2*NUMAXIS);
}

/**
* @brief Return the K matrix
*/
Eigen::Matrix< double, Eigen::Dynamic, Eigen::Dynamic > Sckf::getKalmanGain()
{
    return this->K;
}

/**
* @brief Return the K associated to the attitude
*/
Eigen::Matrix< double, Eigen::Dynamic, Eigen::Dynamic > Sckf::getAttitudeKalmanGain()
{
    return this->K.block<Sckf::A_STATE_VECTOR_SIZE, A_STATE_VECTOR_SIZE> (2*NUMAXIS, 2*NUMAXIS);
}

/**
* @brief Return the innovation vector
*/
Eigen::Matrix< double, Eigen::Dynamic, 1  > localization::Sckf::getInnovation()
{
    return this->innovation;
}

/**
* @brief This function Initialize Attitude
*/
int Sckf::setAttitude(Eigen::Quaternion< double >& initq)
{
    /** Initial orientation **/
    q4 = initq;
	
    return OK;
    
}

/**
* @brief This function sets the gravity value
*/
void Sckf::setGravity(double g)
{
    this->gtilde << 0, 0, g;
}

/**
* @brief This function set the initial Omega matrix
*/
int Sckf::setOmega(Eigen::Matrix< double, NUMAXIS , 1  >& u)
{
    if (&u != NULL)
    {
	/** Initialization for quaternion integration **/
	oldomega4 << 0,-u(0), -u(1), -u(2),
	    u(0), 0, u(2), -u(1),
	    u(1), -u(2), 0, u(0),
	    u(2), u(1), -u(0), 0;
	
	return OK;
    }

    return ERROR;

}

/**
* @brief Gets the current state vector of the filter
*/
void Sckf::setStatex(Eigen::Matrix< double, Eigen::Dynamic, 1  > &x_0)
{
    x_0.resize(Sckf::X_STATE_VECTOR_SIZE,1);
    this->xki_k = x_0;
}

/**
* @brief Set the Heading angle
*/
void Sckf::setHeading(double yaw)
{
    Eigen::Matrix< double, NUMAXIS , 1> euler;
    Eigen::Vector3d e = Eigen::Matrix3d(q4).eulerAngles(2,1,0);
    
    euler(0) = e[2]; 
    euler(1) = e[1]; 
    euler(2) = yaw; 
    
    q4 = Eigen::Quaternion <double> (Eigen::AngleAxisd(euler[2], Eigen::Vector3d::UnitZ())*
	    Eigen::AngleAxisd(euler[1], Eigen::Vector3d::UnitY()) *
	    Eigen::AngleAxisd(euler[0], Eigen::Vector3d::UnitX()));
    
}

/**
* @brief Conversion Quaternion to DCM (Direct Cosine Matrix) (Alternative to Eigen)
*/
void Quaternion2DCM(Eigen::Quaternion< double >* q, Eigen::Matrix< double, NUMAXIS, NUMAXIS  >*C)
{
    double q0, q1, q2, q3;

    if (C != NULL)
    {
	/** Take the parameters of the quaternion */
	q0 = q->w();
	q1 = q->x();
	q2 = q->y();
	q3 = q->z();
	
	/** Create the DCM matrix from the actual quaternion */
	(*C)(0,0) = 2 * q0 * q0 + 2 * q1 * q1 - 1;
	(*C)(0,1) = 2 * q1 * q2 + 2 * q0 * q3;
	(*C)(0,2) = 2 * q1 * q3 - 2 * q0 * q2;
	(*C)(1,0) = 2 * q1 * q2 - 2 * q0 * q3;
	(*C)(1,1) = 2 * q0 * q0 + 2 * q2 * q2 - 1;
	(*C)(1,2) = 2 * q2 * q3 + 2 * q0 * q1;
	(*C)(2,0) = 2 * q1 * q3 + 2 * q0 * q2;
	(*C)(2,1) = 2 * q2 * q3 - 2 * q0 * q1;
	(*C)(2,2) = 2 * q0 * q0 + 2 * q3 * q3 - 1;	
    }
    
    return;
}


 /**
* @brief This function Initilize the vectors and matrices
*/
void Sckf::Init(Eigen::Matrix< double, Eigen::Dynamic, Eigen::Dynamic >& P_0,
		Eigen::Matrix< double, NUMAXIS , NUMAXIS  >& Rg,
		Eigen::Matrix< double, NUMAXIS , NUMAXIS  >& Qbg,
		Eigen::Matrix< double, NUMAXIS , NUMAXIS  >& Qba, 
		Eigen::Matrix< double, NUMAXIS , NUMAXIS  >& Ra,
		Eigen::Matrix< double, NUMAXIS , NUMAXIS  >& Rat,
		Eigen::Matrix< double, NUMAXIS , NUMAXIS  >& Rm,
		double g, double alpha)
{
    
    /** Set the matrix and vector dimension to the static values of the class **/
    xk_k.resize(Sckf::X_STATE_VECTOR_SIZE,1);
    xki_k.resize(Sckf::X_STATE_VECTOR_SIZE,1);
    
    A.resize(Sckf::A_STATE_VECTOR_SIZE,Sckf::A_STATE_VECTOR_SIZE);
    Fki.resize(Sckf::X_STATE_VECTOR_SIZE,Sckf::X_STATE_VECTOR_SIZE);
    
    Qk.resize(Sckf::X_STATE_VECTOR_SIZE,Sckf::X_STATE_VECTOR_SIZE);
    
    Pk_k.resize(Sckf::X_STATE_VECTOR_SIZE,Sckf::X_STATE_VECTOR_SIZE);
    Pki_k.resize(Sckf::X_STATE_VECTOR_SIZE,Sckf::X_STATE_VECTOR_SIZE);
    
    K.resize(Sckf::X_STATE_VECTOR_SIZE, 2*NUMAXIS);
    
    H1a.resize(NUMAXIS, Sckf::A_STATE_VECTOR_SIZE);
    H2a.resize(NUMAXIS, Sckf::A_STATE_VECTOR_SIZE);
    Hk.resize(2*NUMAXIS, Sckf::X_STATE_VECTOR_SIZE);
    
    /** Resizing dynamic arguments to the correct dimension to avoid matrices errors **/
    P_0.resize(Sckf::X_STATE_VECTOR_SIZE, Sckf::X_STATE_VECTOR_SIZE);
    
    /** Gravitation acceleration **/
    gtilde << 0, 0, g;

    /** Dip angle (alpha is in rad) **/
    mtilde(0) = cos(alpha);
    mtilde(1) = 0;
    mtilde(2) = -sin(alpha);

    /** Kalman filter state, system matrix, error covariance and process noise covariance **/
    xki_k = Matrix <double,Sckf::X_STATE_VECTOR_SIZE,1>::Zero();
    xk_k = xki_k;
    
    /** System matrix F **/
    Fki = Matrix <double,Sckf::X_STATE_VECTOR_SIZE, Sckf::X_STATE_VECTOR_SIZE>::Zero();
    
    /** System matrix A **/
    A = Matrix <double,Sckf::A_STATE_VECTOR_SIZE, Sckf::A_STATE_VECTOR_SIZE>::Zero();      
    A(0,3) = -0.5;A(1,4) = -0.5;A(2,5) = -0.5;
    
    /** Process noise **/
    Qk = Matrix <double,Sckf::X_STATE_VECTOR_SIZE,Sckf::X_STATE_VECTOR_SIZE>::Zero();
    Qk.block <NUMAXIS, NUMAXIS> (NUMAXIS,NUMAXIS) = Ra;
    Qk.block <NUMAXIS, NUMAXIS> (2*NUMAXIS,2*NUMAXIS) = 0.25 * Rg;
    Qk.block <NUMAXIS, NUMAXIS> ((2*NUMAXIS)+NUMAXIS,(2*NUMAXIS)+NUMAXIS) = Qbg;
    Qk.block <NUMAXIS, NUMAXIS> ((4*NUMAXIS),(4*NUMAXIS)) = Qba;

    /** Assign the initial values **/
    Pki_k = P_0;
    Pk_k = Pki_k;
    
    /** Assign the initial value for the measurement matrix of the attitude **/
    H1a = Matrix <double,NUMAXIS,Sckf::A_STATE_VECTOR_SIZE>::Zero();
    H2a = Matrix <double,NUMAXIS,Sckf::A_STATE_VECTOR_SIZE>::Zero();
    H1a(0,6) = 1; H1a(1,7) = 1; H1a(2,8) = 1;

    /** Set the history of noise for the attitude **/
    RHist = Eigen::Matrix <double,NUMAXIS,NUMAXIS*M1>::Zero();
    
    /** Fill noise measurement matrices Rg, Ra, Rat, Rm , etc.. **/
    this->Rg = Rg;
    this->Ra = Ra;
    this->Rat = Rat;
    this->Rm = Rm;
    
    /** Resize the noise measurement matrix to the correct dimension **/
    Rk.resize (2*NUMAXIS, 2*NUMAXIS);
    
    /** Measurement vector **/
    zki.resize(2*NUMAXIS,1);
    zki = Eigen::Matrix<double, 2*NUMAXIS, 1>::Zero();
    
    /** Initial bias **/
    bghat = Matrix <double,NUMAXIS,1>::Zero();
    bahat = Matrix <double,NUMAXIS,1>::Zero();
        
    /** Default omega matrix **/
    oldomega4 << 0 , 0 , 0 , 0,
	0 , 0 , 0 , 0,
	0 , 0 , 0 , 0,
	0 , 0 , 0 , 0;
    
    /** Initial quaternion in Init is NaN**/
    q4.w() = std::numeric_limits<double>::quiet_NaN();
    q4.x() = std::numeric_limits<double>::quiet_NaN();
    q4.y() = std::numeric_limits<double>::quiet_NaN();
    q4.z() = std::numeric_limits<double>::quiet_NaN();
    
    /** Default initial bias **/
    bghat << 0.00, 0.00, 0.00;
    bahat << 0.00, 0.00, 0.00;
    
    /** Variable in the adaptive algorithm **/
    r1count = 0;
    r2count = R2COUNT;
        
    /** Print filter information **/
    #ifdef DEBUG_PRINTS
    std::cout<< "xki_k is of size "<<xki_k.rows()<<"x"<<xki_k.cols()<<"\n";
    std::cout<< "xki_k:\n"<<xki_k<<"\n";
    std::cout<< "xk_k is of size "<<xk_k.rows()<<"x"<<xk_k.cols()<<"\n";
    std::cout<< "xk_k:\n"<<xk_k<<"\n";
    std::cout<< "A is of size "<<A.rows()<<"x"<<A.cols()<<"\n";
    std::cout<< "A:\n"<<A<<"\n";
    std::cout<< "Fki is of size "<<Fki.rows()<<"x"<<Fki.cols()<<"\n";
    std::cout<< "Fki:\n"<<Fki<<"\n";
    std::cout<< "Pk+i|k is of size "<<Pki_k.rows()<<"x"<<Pki_k.cols()<<"\n";
    std::cout<< "Pk+i|k:\n"<<Pki_k<<"\n";
    std::cout<< "Pk|k is of size "<<Pk_k.rows()<<"x"<<Pk_k.cols()<<"\n";
    std::cout<< "Pk|k:\n"<<Pk_k<<"\n";
    std::cout<< "Qk|k is of size "<<Qk.rows()<<"x"<<Qk.cols()<<"\n";
    std::cout<< "Qk|k:\n"<<Qk<<"\n";
    std::cout<< "H1a is of size "<<H1a.rows()<<"x"<<H1a.cols()<<"\n";
    std::cout<< "H1a:\n"<<H1a<<"\n";
    std::cout<< "H2a is of size "<<H2a.rows()<<"x"<<H2a.cols()<<"\n";
    std::cout<< "H2a:\n"<<H2a<<"\n";
    std::cout<< "zki is of size "<<zki.rows()<<"x"<<zki.cols()<<"\n";
    std::cout<< "zki:\n"<<zki<<"\n";
    std::cout<< "RHist is of size "<<RHist.rows()<<"x"<<RHist.cols()<<"\n";
    std::cout<< "RHist:\n"<<RHist<<"\n";
    std::cout<< "Rg is of size "<<Rg.rows()<<"x"<<Rg.cols()<<"\n";
    std::cout<< "Rg:\n"<<Rg<<"\n";
    std::cout<< "Ra is of size "<<Ra.rows()<<"x"<<Ra.cols()<<"\n";
    std::cout<< "Ra:\n"<<Ra<<"\n";
    std::cout<< "Rm is of size "<<Rm.rows()<<"x"<<Rm.cols()<<"\n";
    std::cout<< "Rm:\n"<<Rm<<"\n";
    std::cout<< "mtilde is of size "<<mtilde.rows()<<"x"<<mtilde.cols()<<"\n";
    std::cout<< "mtilde:\n"<<mtilde<<"\n";
    std::cout<< "gtilde is of size "<<gtilde.rows()<<"x"<<gtilde.cols()<<"\n";
    std::cout<< "gtilde:\n"<<gtilde<<"\n";
    std::cout<< "bghat is of size "<<bghat.rows()<<"x"<<bghat.cols()<<"\n";
    std::cout<< "bghat:\n"<<bghat<<"\n";
    std::cout<< "bahat is of size "<<bahat.rows()<<"x"<<bahat.cols()<<"\n";
    std::cout<< "bahat:\n"<<bahat<<"\n";
    #endif

}

/**
* @brief Performs the prediction step of the filter.
*/
void Sckf::predict(Eigen::Matrix< double, NUMAXIS , 1  >& u, Eigen::Matrix< double, NUMAXIS , 1  >& v, double dt)
{
    Eigen::Matrix <double,NUMAXIS,NUMAXIS> Cq; /** Rotational matrix */
    Eigen::Matrix <double,NUMAXIS,NUMAXIS> velo2product; /** Vec 2 product  matrix */
    Eigen::Matrix <double,NUMAXIS,NUMAXIS> acc2product; /** Vec 2 product  matrix */
    Eigen::Matrix <double,NUMAXIS,1> angvelo; /** Angular velocity */
    Eigen::Matrix <double,NUMAXIS,1> linacc; /** Linear acceleration */
    Eigen::Matrix <double,NUMAXIS,1> gtilde_body; /** Gravitation in the body frame */
    Eigen::Matrix <double,QUATERSIZE,QUATERSIZE> omega4; /** Quaternion integration matrix */
    Eigen::Matrix <double,QUATERSIZE,1> quat; /** Quaternion integration matrix */
    Eigen::Matrix <double,Sckf::X_STATE_VECTOR_SIZE,Sckf::X_STATE_VECTOR_SIZE> dFki; /** Discrete System matrix */
    Eigen::Matrix <double,X_STATE_VECTOR_SIZE,X_STATE_VECTOR_SIZE> Qdk; /** Discrete Qk matrix */

    /** Compute the cross product matrix with the angular velocity **/
    angvelo = u - bghat; /** Eliminate the Bias **/
    filtermeasurement.setAngularVelocities(angvelo); /** Save it in the global variable **/

    /** In cross product form **/
    velo2product << 0, -angvelo(2), angvelo(1),
		angvelo(2), 0, -angvelo(0),
		-angvelo(1), angvelo(0), 0;

    /** Create the orientation matrix from the quaternion **/
    Quaternion2DCM (&q4, &Cq);
    
    /** Calculate the gravity vector in the body frame **/
    gtilde_body = Cq * gtilde;
		
    /** Compute the cross product matrix with the linear acceleration **/
    linacc = v - bahat - gtilde_body; /** Eliminate the Bias and the local gravity vector **/
    filtermeasurement.setLinearAcceleration(linacc); /** Save it in the measurement **/
    
    #ifdef DEBUG_PRINTS
    std::cout<<"[Predict] angevelo:\n"<<angvelo<<"\n";
    std::cout<<"[Predict] gtilde_body of size "<<gtilde_body.rows()<<"x"<<gtilde_body.cols()<<"\n";
    std::cout<<"[Predict] g in body_frame:\n"<<gtilde_body<<"\n";
    std::cout<<"[Predict] linacc:\n"<<linacc<<"\n";
    #endif

    /** In cross product form **/
    acc2product << 0, -linacc(2), linacc(1),
		linacc(2), 0, -linacc(0),
		-linacc(1), linacc(0), 0;
		
    /** Compute the dA Matrix of the attitude part **/
    A.block<NUMAXIS, NUMAXIS> (0,0) = -velo2product;
    
    /** Form the complete system model matrix (position error) **/
    Fki.block<NUMAXIS, NUMAXIS> (0,NUMAXIS) = Eigen::Matrix <double,NUMAXIS, NUMAXIS>::Identity();
    
    /** Velocity part **/
    Fki.block<NUMAXIS, NUMAXIS> (NUMAXIS, 2*NUMAXIS) = -Cq.inverse() * acc2product;
    Fki.block<NUMAXIS, NUMAXIS> (NUMAXIS, 4*NUMAXIS) = -Cq.inverse();
    
    /** Attitude part **/
    Fki.block<Sckf::A_STATE_VECTOR_SIZE, Sckf::A_STATE_VECTOR_SIZE>(2*NUMAXIS, 2*NUMAXIS) = A;

    /** Discretization of the linear system **/
    dFki = Eigen::Matrix<double,Sckf::X_STATE_VECTOR_SIZE,Sckf::X_STATE_VECTOR_SIZE>::Identity() + Fki * dt + Fki * Fki * pow(dt,2)/2.0;
    
    #ifdef DEBUG_PRINTS
    std::cout<< "[Predict] xki|k is of size "<<xki_k.rows()<<"x"<<xki_k.cols()<<"\n";
    std::cout<< "[Predict] xki_k:\n"<<xki_k<<"\n";
    #endif
    
    /** Propagate the vector through the system **/
    xki_k = dFki * xki_k;
    
    #ifdef DEBUG_PRINTS
    std::cout<< "[After Predict] xki_k:\n"<<xki_k<<"\n";
    std::cout<< "[Predict] Fki is of size "<<Fki.rows()<<"x"<<Fki.cols()<<"\n";
    std::cout<< "[Predict] Fki:\n"<<Fki<<"\n";
    std::cout<< "[Predict] dFki is of size "<<dFki.rows()<<"x"<<dFki.cols()<<"\n";
    std::cout<< "[Predict] dFki:\n"<<dFki<<"\n";
    #endif
    
    /** The noise process noise related to teh velocity is depending on the current attitude **/
    Qk.block <NUMAXIS, NUMAXIS> (0,0) = Ra * dt;
    Qk.block <NUMAXIS, NUMAXIS> (NUMAXIS,NUMAXIS) = Cq.inverse() * this->Ra;
    
    /** Form the system noise matrix for the attitude **/
    Qdk = Qk*dt + 0.5*dt*dt*Fki*Qk + 0.5*dt*dt*Qk*Fki.transpose();
    Qdk = 0.5*(Qdk + Qdk.transpose());
    
    Pki_k = dFki*Pki_k*dFki.transpose() + Qdk;
    
    #ifdef DEBUG_PRINTS
    std::cout<< "[Predict] Qdk is of size "<<Qdk.rows()<<"x"<<Qdk.cols()<<"\n";
    std::cout<< "[Predict] Qdk:\n"<<Qdk<<"\n";
    std::cout<< "[Predict] Pki_k:\n"<<Pki_k<<"\n";
    #endif
        
    omega4 << 0,-angvelo(0), -angvelo(1), -angvelo(2),
	    angvelo(0), 0, angvelo(2), -angvelo(1),
	    angvelo(1), -angvelo(2), 0, angvelo(0),
	    angvelo(2), angvelo(1), -angvelo(0), 0;
	    
    quat(0) = q4.w();
    quat(1) = q4.x();
    quat(2) = q4.y();
    quat(3) = q4.z();

    /** Quaternion integration **/
    quat = (Matrix<double,QUATERSIZE,QUATERSIZE>::Identity() +(0.75 * omega4 *dt)- (0.25 * oldomega4 * dt) -
    ((1/6) * angvelo.squaredNorm() * pow(dt,2) *  Matrix<double,QUATERSIZE,QUATERSIZE>::Identity()) -
    ((1/24) * omega4 * oldomega4 * pow(dt,2)) - ((1/48) * angvelo.squaredNorm() * omega4 * pow(dt,3))) * quat;

    /** Store in a quaternion form **/
    q4.w() = quat(0);
    q4.x() = quat(1);
    q4.y() = quat(2);
    q4.z() = quat(3);
    q4.normalize();

    oldomega4 = omega4;

    return;

}


void Sckf::update(Eigen::Matrix <double,NUMAXIS,SLIP_VECTOR_SIZE> &Hme, Eigen::Matrix <double,SLIP_VECTOR_SIZE,SLIP_VECTOR_SIZE> &Rme,
		  Eigen::Matrix< double, SLIP_VECTOR_SIZE, 1  >& slip_error,
		  Eigen::Matrix< double, NUMAXIS , 1  >& acc,Eigen::Matrix< double, NUMAXIS , 1  >& mag, double dt, bool magn_on_off)
{
    register int j;
    Eigen::Matrix <double,NUMAXIS,NUMAXIS> Cq; /** Rotational matrix */
    Eigen::Matrix <double,NUMAXIS,NUMAXIS> gtilde2product; /** Vec 2 product  matrix for the gravity vector in body frame*/
    Eigen::Matrix <double,NUMAXIS,NUMAXIS> fooR2; /**  Measurement noise matrix from accelerometers matrix Ra*/
    Eigen::Matrix <double,A_STATE_VECTOR_SIZE,1> xa_k; /** Attitude part of the state vector xk+i|k */
    Eigen::Matrix <double,A_STATE_VECTOR_SIZE,A_STATE_VECTOR_SIZE> P1a; /** Error convariance matrix for measurement 1 of the attitude */
    Eigen::Matrix <double,A_STATE_VECTOR_SIZE,A_STATE_VECTOR_SIZE> P2a; /** Error convariance matrix for measurement 2 of the attitude */
    Eigen::Matrix <double,A_STATE_VECTOR_SIZE,A_STATE_VECTOR_SIZE> auxM; /** Auxiliar matrix for computing Kalman gain in measurement */
    Eigen::Matrix <double,NUMAXIS,NUMAXIS> Rve; /** Measurement noise convariance matrix for velocity error */
    Eigen::Matrix <double,NUMAXIS,NUMAXIS> R1a; /** Measurement noise convariance matrix for measurement 1 */
    Eigen::Matrix <double,NUMAXIS,NUMAXIS> Uk; /** Uk measurement noise convariance matrix for the adaptive algorithm */
    Eigen::Matrix <double,NUMAXIS,NUMAXIS> Qstar; /** External acceleration covariance matrix */
    Eigen::Quaternion <double> qe;  /** Attitude error quaternion */
    Eigen::Matrix <double,NUMAXIS,1> gtilde_body; /** Gravitation in the body frame */
    Eigen::Matrix <double,NUMAXIS,1> mtilde_body; /** Magnetic field in the body frame */
    Eigen::Matrix <double,NUMAXIS,NUMAXIS> u; /** Unitary matrix U for the SVD decomposition */
    Eigen::Matrix <double,NUMAXIS,1> s; /** Unitary matrix V for the SVD decomposition */
    Eigen::Matrix <double,NUMAXIS,1> lambda; /** Lambda vector for the adaptive algorithm */
    Eigen::Matrix <double,NUMAXIS,1> mu; /** mu vector for the adaptive algorithm */
    Eigen::Matrix <double,NUMAXIS,1> auxvector; /** Auxiliar vector variable */
    Eigen::Matrix <double,NUMAXIS,1> z1a; /** Measurement vector 1 Acc */
    Eigen::Matrix <double,NUMAXIS,1> z2a; /** Measurement vector 2 Mag */
    
    
    /** Copy the attitude part of the state vector and covariance matrix **/
    xa_k = xki_k.block<Sckf::A_STATE_VECTOR_SIZE, 1> (2*NUMAXIS, 0);
    P1a = Pki_k.block<Sckf::A_STATE_VECTOR_SIZE, Sckf::A_STATE_VECTOR_SIZE> (2*NUMAXIS, 2*NUMAXIS);
    
    #ifdef DEBUG_PRINTS
    std::cout<<"[Update] xa_k is of size "<<xa_k.rows()<<"x"<<xa_k.cols()<<"\n";
    std::cout<<"[Update] xa_k:\n"<<xa_k<<"\n";
    std::cout<<"[Update] P1a is of size "<<P1a.rows()<<"x"<<P1a.cols()<<"\n";
    std::cout<<"[Update] P1a:\n"<<P1a<<"\n";
    #endif
    
    /** Create the orientation matrix from the quaternion **/
    Quaternion2DCM (&q4, &Cq);
    
    /** Calculate the gravity vector in the body frame **/
    gtilde_body = Cq * gtilde;
    
    gtilde2product << 0, -gtilde_body(2), gtilde_body(1),
		    gtilde_body(2), 0, -gtilde_body(0),
		    -gtilde_body(1), gtilde_body(0), 0;

    #ifdef DEBUG_PRINTS
    std::cout<<"[Update] gtilde_body is of size "<<gtilde_body.rows()<<"x"<<gtilde_body.cols()<<"\n";
    std::cout<<"[Update] g in body_frame:\n"<<gtilde_body<<"\n";
    #endif

    /** Form the matrix for the measurement 1 of the attitude (acc correction) **/
    H1a.block<NUMAXIS, NUMAXIS> (0,0) = 2*gtilde2product;
    
    /** Form the measurement vector z1a for the attitude (the real acceleration value) **/
    z1a = acc - bahat - gtilde_body;
    
    /** The adaptive algorithm for the attitude, the Uk matrix and SVD part **/
    R1a = (z1a - H1a*xa_k) * (z1a - H1a*xa_k).transpose();
    RHist.block <NUMAXIS, NUMAXIS> (0, (r1count*(M1-1))%M1) = R1a;
    
    /** r1count + 1 modulus the number of history M1 **/
    r1count++; 

    /** Starting the Uk is R **/
    Uk = R1a;
    for (j=0; j<M1; j++)
    {
	Uk = Uk + RHist.block <NUMAXIS, NUMAXIS> (0,NUMAXIS*j);
    }
    
    Uk = Uk/static_cast<double>(M1);
    
    fooR2 = H1a*P1a*H1a.transpose() + Ra;
    
    /**
    * Single Value Decomposition
    */
    JacobiSVD <MatrixXd > svdOfR1a(Uk, ComputeThinU);

    s = svdOfR1a.singularValues();
    u = svdOfR1a.matrixU();
    
    lambda << s(0), s(1), s(2);
    
    mu(0) = (u.transpose().row(0) * fooR2).dot(u.col(0));
    mu(1) = (u.transpose().row(1) * fooR2).dot(u.col(1));
    mu(2) = (u.transpose().row(2) * fooR2).dot(u.col(2));
    
    if ((lambda - mu).maxCoeff() > GAMMA)
    {
	#ifdef DEBUG_PRINTS
	std::cout<<"[Update] "<<(lambda - mu).maxCoeff()<<" Bigger than GAMMA\n";
	#endif
	
	r2count = 0;
	auxvector(0) = std::max(lambda(0)-mu(0),(double)0.00);
	auxvector(1) = std::max(lambda(1)-mu(1),(double)0.00);
	auxvector(2) = std::max(lambda(2)-mu(2),(double)0.00);
	
	Qstar = auxvector(0) * u.col(0) * u.col(0).transpose() + auxvector(1) * u.col(1) * u.col(1).transpose() + auxvector(2) * u.col(2) * u.col(2).transpose();
    }
    else
    {
	#ifdef DEBUG_PRINTS
	std::cout<<"[Update] r2count: "<<r2count<<"\n";
	#endif
	
	r2count ++;
	if (r2count < M2)
	    Qstar = auxvector(0) * u.col(0) * u.col(0).transpose() + auxvector(1) * u.col(1) * u.col(1).transpose() + auxvector(2) * u.col(2) * u.col(2).transpose();
	else
	    Qstar = Matrix<double, NUMAXIS, NUMAXIS>::Zero();
    }
    
    /** Measurement vector **/
    zki.block<NUMAXIS, 1> (0,0) = Hme * slip_error;
    zki.block<NUMAXIS, 1> (NUMAXIS,0) = z1a;
    
    /** Form the observation matrix Hk **/
    Hk = Eigen::Matrix<double, 2*NUMAXIS, X_STATE_VECTOR_SIZE>::Zero();
    Hk.block<NUMAXIS, NUMAXIS>(0,NUMAXIS) = Cq;
    Hk.block<NUMAXIS, A_STATE_VECTOR_SIZE>(NUMAXIS,2*NUMAXIS) = H1a;
    
    /** Define the velocity vector measurement noise from the slip_error vector measurement**/
    Rve = Hme * Rme * Hme.transpose();
    
    /** Define the attitude measurement noise **/
    R1a = Ra + Rat + Qstar; //Qstart is the external acceleration covariance
    
    /** Composition of the measurement noise matrix **/
    Rk.block<NUMAXIS, NUMAXIS> (0,0) = Rve;
    Rk.block<NUMAXIS, NUMAXIS> (NUMAXIS,NUMAXIS) = R1a;
    
    /** Compute the Kalman Gain Matrix **/
    K = Pki_k * Hk.transpose() * (Hk * Pki_k * Hk.transpose() + Rk).inverse();
      
    /** Update the state vector and the covariance matrix **/
    xki_k = xki_k + K * (zki - Hk * xki_k);    
    Pki_k = (Matrix<double,Sckf::X_STATE_VECTOR_SIZE,Sckf::X_STATE_VECTOR_SIZE>::Identity()-K*Hk)*Pki_k*(Matrix<double,Sckf::X_STATE_VECTOR_SIZE,Sckf::X_STATE_VECTOR_SIZE>::Identity()-K*Hk).transpose() + K*Rk*K.transpose();
    Pki_k = 0.5 * (Pki_k + Pki_k.transpose());
    
    #ifdef DEBUG_PRINTS
    std::cout<< "[Update] Qstar is of size "<<Qstar.rows()<<"x"<<Qstar.cols()<<"\n";
    std::cout<< "[Update] Qstar:\n"<<Qstar<<"\n";
    std::cout<< "[Update] R1a is of size "<<R1a.rows()<<"x"<<R1a.cols()<<"\n";
    std::cout<< "[Update] R1a:\n"<<R1a<<"\n";
    std::cout<< "[Update] xa_k is of size "<<xa_k.rows()<<"x"<<xa_k.cols()<<"\n";
    std::cout<< "[Update] xa_k:\n"<<xa_k<<"\n";
    std::cout<< "[Update] P1a is of size "<<P1a.rows()<<"x"<<P1a.cols()<<"\n";
    std::cout<< "[Update] P1a:\n"<<P1a<<"\n";
    std::cout<< "[Update] Hk is of size "<<Hk.rows()<<"x"<<Hk.cols()<<"\n";
    std::cout<< "[Update] Hk:\n"<<Hk<<"\n";
    std::cout<< "[Update] xki_k is of size "<<xki_k.rows()<<"x"<<xki_k.cols()<<"\n";
    std::cout<< "[Update] xki_k:\n"<<xki_k<<"\n";
    std::cout<< "[Update] Pki_k is of size "<<Pki_k.rows()<<"x"<<Pki_k.cols()<<"\n";
    std::cout<< "[Update] Pki_k:\n"<<Pki_k<<"\n";
    std::cout<< "[Update] K is of size "<<K.rows()<<"x"<<K.cols()<<"\n";
    std::cout<< "[Update] K:\n"<<K<<"\n";
    
    #endif
    
    /** Update the quaternion with the Indirect approach **/
    qe.w() = 1;
    qe.x() = xa_k(0);
    qe.y() = xa_k(1);
    qe.z() = xa_k(2);
    
    Eigen::Matrix <double,NUMAXIS,1> error_euler; /** The error in euler angles **/
    error_euler[2] = 0.00; //Dont update the yaw
    error_euler[1] = qe.toRotationMatrix().eulerAngles(2,1,0)[1];//PITCH
    error_euler[0] = qe.toRotationMatrix().eulerAngles(2,1,0)[2];//ROLL
    
    qe = Eigen::Quaternion <double> (Eigen::AngleAxisd(error_euler[0], Eigen::Vector3d::UnitX())*
			Eigen::AngleAxisd(error_euler[1], Eigen::Vector3d::UnitY()) *
			Eigen::AngleAxisd(error_euler[2], Eigen::Vector3d::UnitZ()));
    
    /** Correct the attitude using the error quaternion **/
    q4 = q4 * qe;
    
    /** Normalize quaternion **/
    q4.normalize();
    
    /** Update the bias with the bias error **/
    bghat = bghat + xki_k.block<NUMAXIS, 1> (3*NUMAXIS,0);
    bahat = bahat + xki_k.block<NUMAXIS, 1> (4*NUMAXIS,0);
    
    #ifdef DEBUG_PRINTS
    Eigen::Matrix <double,NUMAXIS,1> euler; /** In euler angles **/
    euler[2] = qe.toRotationMatrix().eulerAngles(2,1,0)[0];//YAW
    euler[1] = qe.toRotationMatrix().eulerAngles(2,1,0)[1];//PITCH
    euler[0] = qe.toRotationMatrix().eulerAngles(2,1,0)[2];//ROLL
    std::cout<< "[Update] Error quaternion in euler\n";
    std::cout<< "[Update] Roll: "<<euler[0]*R2D<<" Pitch: "<<euler[1]*R2D<<" Yaw: "<<euler[2]*R2D<<"\n";
    std::cout<< "[Update] bghat is of size "<<bghat.rows()<<"x"<<bghat.cols()<<"\n";
    std::cout<< "[Update] bghat:\n"<<bghat<<"\n";
    std::cout<< "[Update] bahat is of size "<<bahat.rows()<<"x"<<bahat.cols()<<"\n";
    std::cout<< "[Update] bahat:\n"<<bahat<<"\n";
    #endif

    return;

}

void Sckf::measurementGeneration(const Eigen::Matrix< double, Eigen::Dynamic, Eigen::Dynamic >& Anav, const Eigen::Matrix< double, Eigen::Dynamic, Eigen::Dynamic >& Bnav,
				 const Eigen::Matrix< double,Eigen::Dynamic, Eigen::Dynamic >& Aslip, const Eigen::Matrix< double, Eigen::Dynamic, Eigen::Dynamic >& Bslip,
				 Eigen::Matrix< double, Eigen::Dynamic, 1  > &vjoints, double dt)
{
    Eigen::Matrix <double, Eigen::Dynamic, Eigen::Dynamic> Rm; /** Measurement Noise matrix of the measurement vector of the LS**/
    Eigen::Matrix <double, Eigen::Dynamic, Eigen::Dynamic> R; /** Measurement Noise matrix of the observation vector of the LS**/
    Eigen::Matrix< double, NUMAXIS , 1  > linvelo; /** Linear velocities from acc integration **/
    
    R.resize(NUMBER_OF_WHEELS*(2*NUMAXIS),NUMBER_OF_WHEELS*(2*NUMAXIS));
    
    #ifdef DEBUG_PRINTS
    std::cout<<"********** MEASUREMENT GENERATION *****************\n";
    #endif
	
    /** Form the noise matrix for the velocity model (navigationKinematics) **/
    Rm.resize(NUMAXIS+ENCODERS_VECTOR_SIZE, NUMAXIS+ENCODERS_VECTOR_SIZE);
    Rm.setZero();
//     R.block<NUMAXIS, NUMAXIS>(0,0) = this->Rg;
//     R.block<ENCODERS_VECTOR_SIZE, ENCODERS_VECTOR_SIZE>(NUMAXIS,NUMAXIS) = filtermeasurement.getEncodersVelocityCovariance();
    Rm.setIdentity();
    R.setIdentity();
    
    /** Set encoders velocity **/
    filtermeasurement.setEncodersVelocity(vjoints);
    
    /** Call the Navigation kinematics to know the velocity from odometry, and contact angles velocities **/
    filtermeasurement.navigationKinematics(Anav, Bnav, R);
    
    /** Integrate accelerometers to have velocity **/
    linvelo = filtermeasurement.accIntegrationWindow(dt);
    filtermeasurement.setLinearVelocities(linvelo);
    
    /** Form the noise matrix for the slip model (slipKinematics) **/
    Rm.resize(2*NUMAXIS + ENCODERS_VECTOR_SIZE + NUMBER_OF_WHEELS, 2*NUMAXIS + ENCODERS_VECTOR_SIZE + NUMBER_OF_WHEELS);
    Rm.setZero();
    Rm.setIdentity();
    R.setIdentity();
    
    /** Compute the Slip Kinematics **/
    filtermeasurement.slipKinematics(Aslip, Bslip, R);
    
    /** Compute the slip vector error **/

}


void Sckf::resetStateVector()
{

    /**------------------------------ **/
    /** Reset some parts of the state **/
    /**------------------------------ **/
    
    /** Reset the quaternion part of the state vector (the error quaternion) **/
    xki_k.block<NUMAXIS,1>(2*NUMAXIS,0) = Matrix<double, NUMAXIS, 1>::Zero();
    
    /** Reset the gyroscopes bias **/
    xki_k.block<NUMAXIS, 1> ((3*NUMAXIS),0) = Matrix <double, NUMAXIS, 1>::Zero();
    
    /** Reset the accelerometers bias **/
    xki_k.block<NUMAXIS, 1> ((4*NUMAXIS),0) = Matrix <double, NUMAXIS, 1>::Zero();
    
    #ifdef DEBUG_PRINTS
    std::cout<< "[Update After Reset] xki_k is of size "<<xki_k.rows()<<"x"<<xki_k.cols()<<"\n";
    std::cout<< "[Update After Reset] xki_k:\n"<<xki_k<<"\n";
    std::cout<< "[Update] bghat is of size "<<bghat.rows()<<"x"<<bghat.cols()<<"\n";
    std::cout<< "[Update] bghat:\n"<<bghat<<"\n";
    std::cout<< "[Update] bahat is of size "<<bahat.rows()<<"x"<<bahat.cols()<<"\n";
    std::cout<< "[Update] bahat:\n"<<bahat<<"\n";
    #endif
    
    return;
}
