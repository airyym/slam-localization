/**\file DataModel.cpp
 *
 * This class has the primitive methods for the data Model 
 *  of the measurement generation
 * 
 * 
 * @author Javier Hidalgo Carrio | DFKI RIC Bremen | javier.hidalgo_carrio@dfki.de
 * @date December 2012.
 * @version 1.0.
 */

#include "DataModel.hpp" 

using namespace localization;


DataModel::DataModel()
{
    data.resize(1,1);
    data.setZero();
    Cov.resize(1,1);
    Cov.setIdentity();
}

DataModel::DataModel(const unsigned int dim)
{
    data.resize(dim,1);
    data.setZero();
    Cov.resize(dim,dim);
    Cov.setIdentity();
    
}

DataModel::DataModel(Eigen::Matrix< double, Eigen::Dynamic, 1  >& data, Eigen::Matrix< double, Eigen::Dynamic, Eigen::Dynamic > &Cov)
{
    this->data.resize(data.size(),1);
    this->data = data;
    
    this->Cov.resize(Cov.size(), Cov.size());
    this->Cov = Cov;

}



DataModel DataModel::operator+(const DataModel& data1) const
{
    const DataModel &data2 (*this);
    
    if ((data1.Cov.size() != 0 && data2.Cov.size() != 0) && (data1.Cov.size() == data2.Cov.size()))
    {
	Eigen::Matrix <double, Eigen::Dynamic, 1> result;
	Eigen::Matrix <double, Eigen::Dynamic, Eigen::Dynamic> P;
	
	result.resize(data1.data.size(), 1);
	P.resize(data1.Cov.size(), data1.Cov.size());
	
	P = (data1.Cov.inverse() + data2.Cov.inverse()).inverse();
	result = P *(data1.Cov.inverse() * data1.data + data2.Cov.inverse() * data2.data);
	
	return DataModel(result, P);
    }
    
    return data1;

}

DataModel& DataModel::operator=(const DataModel& dmodel)
{
    data = dmodel.data;
    Cov = dmodel.Cov;
    
    return *this;
}


std::ostream& localization::operator<<(std::ostream &out, const  DataModel& data1)
{
    out << data1.data << "\n";
    out << data1.Cov << "\n";
    
    return out;
}





