#include <plan_manage/generator.h>
#include <iostream>

using namespace std;
Traj TrajGenerator::Generator(const MatrixXd& waypoints){

    int n = waypoints.cols();
    Traj res;
    MatrixXd d = waypoints.block(0, 1, 3, n - 1) - waypoints.block(0, 0, 3, n - 1);
    MatrixXd d_square = (d.row(0).array()).pow(2) + (d.row(1).array()).pow(2) + (d.row(2).array()).pow(2);
 
    MatrixXd d0 = 2 * d_square.array().sqrt();

    for(int i = 1; i < d0.cols(); i++){
        d0(i) = d0(i - 1) + d0(i);
    }
  
    traj_time_.resize(n);
    traj_time_[0] = 0;
    for(int i = 1; i < n; i++){
        traj_time_[i] = d0(i - 1);
    }
    std::vector<Eigen::MatrixXd> Polynomial_coefficients;
    Polynomial_coefficients.push_back(CalcuCoef(waypoints.row(0)));
    Polynomial_coefficients.push_back(CalcuCoef(waypoints.row(1)));
    Polynomial_coefficients.push_back(CalcuCoef(waypoints.row(2)));
    int size = traj_time_.size();
    res.pos.resize(traj_time_[size - 1] / T_);
    res.vel.resize(traj_time_[size - 1] / T_);
    res.acc.resize(traj_time_[size - 1] / T_);
    for(int i = 0; i < res.pos.size(); i++){
        double t = T_ * i;
        vector<Vector3d>  pva = PolyFunc(t, Polynomial_coefficients);
        res.pos[i] = pva[0];
        res.vel[i] = pva[1];
        res.acc[i] = pva[2];
    }
    return res;
}

void TrajGenerator::RandomGenerator(){
    Eigen::Matrix3d waypoints = Eigen::Matrix3d::Random() * 0.4;

    Traj traj_ = Generator(waypoints);
    traj_pos_ = traj_.pos;
    traj_vel_ = traj_.vel;
    traj_acc_ = traj_.acc;
}
MatrixXd TrajGenerator::CalcuCoef(const MatrixXd& waypoints){
    MatrixXd alpha;
    int n = waypoints.size() - 1;
    MatrixXd b = MatrixXd::Zero(8 * n, 1);
    MatrixXd A = MatrixXd::Zero(8 * n, 8 * n);
    int row_index = 0;
    for(int i = 0; i < n; i++){
        b(row_index) = waypoints(i);
        A.row(row_index++) = GetMatrixLine(i, 0, 0);
        b(row_index) = waypoints(i + 1);
        A.row(row_index++) = GetMatrixLine(i, 0, 1);
        
    }
    for(int k = 1; k <= 3; k++){
        A.row(row_index++) = GetMatrixLine(0, k, 0);
        A.row(row_index++) = GetMatrixLine(n - 1, k, 1);
    }
    for(int i = 0; i < n - 1; i++){
        for(int k = 1; k <= 6; k++){
            A.row(row_index) = GetMatrixLine(i + 1, k, 0);
            A.block<1, 8>(row_index, 8 * i) =  - GetMatrixLine(i, k, 1).block<1, 8>(0, 8 * i); 
            b(row_index++) = 0;
        }
    }
    alpha = A.colPivHouseholderQr().solve(b);
    // alpha = (A.inverse() * b).transpose();
    return alpha;
}

MatrixXd TrajGenerator::GetMatrixLine(int piecewise_th, int order, double start_or_end){
    int n = traj_time_.size() - 1; //   ??????n???
    int t_index = piecewise_th;
    double s = traj_time_[t_index];
    double T = traj_time_[t_index + 1] - s;
    double t;
    if(start_or_end == 0){
        t = s;
    }else{
        t = traj_time_[t_index + 1];
    }
    Matrix<double, 7, 8> M ;  
    MatrixXd res = MatrixXd::Zero(1, 8 * n);
    
    M << 1,    ((t-s)/T),  pow(((t-s)/T), 2),  pow((t-s)/T, 3),    pow((t-s)/T, 4), pow((t-s)/T, 5),  pow((t-s)/T, 6),  pow((t-s)/T, 7),\
        0,    (1/T),      2*((t-s)/T)/ T, 3*pow((t-s)/T, 2) / T,    4*pow((t-s)/T, 3) /T,    5*pow((t-s)/T, 4)/T,  6*pow((t-s)/T, 5)/T, 7*pow((t-s)/T, 6) /T,\
        0,      0,         2/pow(T, 2),    2*3*((t-s)/T)/pow(T, 2),    3*4*pow((t-s)/T, 2) /pow(T, 2),    4*5*pow((t-s)/T, 3)/pow(T, 2), 5*6*pow((t-s)/T, 4) /pow(T, 2), 6*7*pow((t-s)/T, 5) /pow(T, 2),\
        0,      0,         0,           2*3/pow(T, 3),    2*3*4*((t-s)/T) /pow(T, 3),  3*4*5*pow((t-s)/T, 2)/pow(T, 3),   4*5*6*pow((t-s)/T, 3) /pow(T, 3),     5*6*7*pow((t-s)/T, 4)/pow(T, 3) ,\
        0,      0,         0,           0,              2*3*4 /pow(T, 4),       2*3*4*5*((t-s)/T)/pow(T, 4),    3*4*5*6*pow((t-s)/T, 2) /pow(T, 4),     4*5*6*7*pow((t-s)/T, 3) /pow(T, 4) ,\
        0,      0,         0,           0,              0,        2*3*4*5/pow(T, 5),    2*3*4*5*6*((t-s)/T) /pow(T, 5),      3*4*5*6*7*pow((t-s)/T, 2) /pow(T, 5),\
        0,      0,         0,           0,              0,        0,    2*3*4*5*6 /pow(T, 6),     2*3*4*5*6*7*((t-s)/T) /pow(T, 6);
    //std::cout << M << std::endl;
    res.block<1, 8>(0, piecewise_th * 8) = M.row(order);
    return res;
}

vector<Vector3d> TrajGenerator::PolyFunc(double t, std::vector<Eigen::MatrixXd>& Polynomial_coefficients){
    int n = traj_time_.size() - 1;  //n????????????
    if(t >= traj_time_[n]){
        t = traj_time_[n];
    }
    int t_index = 0;
    for(int i = 0; i < n; i++){
        if(t < traj_time_[i])   break;
        t_index = i;
    }
    double s = traj_time_[t_index];
    double T = traj_time_[t_index + 1] - s;
    vector<Vector3d> pva;
    Vector3d p, v, a;
    Matrix<double, 1, 8> poly_p, poly_v, poly_a;  
    poly_p << 1,    ((t-s)/T),  pow(((t-s)/T), 2),  pow((t-s)/T, 3),    pow((t-s)/T, 4), pow((t-s)/T, 5),  pow((t-s)/T, 6),  pow((t-s)/T, 7);
    poly_v << 0,    (1/T),      2*((t-s)/T)/ T, 3*pow((t-s)/T, 2) / T,    4*pow((t-s)/T, 3) /T,    5*pow((t-s)/T, 4)/T,  6*pow((t-s)/T, 5)/T, 7*pow((t-s)/T, 6) /T;
    poly_a << 0,      0,         2/pow(T, 2),    2*3*((t-s)/T)/pow(T, 2),    3*4*pow((t-s)/T, 2) /pow(T, 2),    4*5*pow((t-s)/T, 3)/pow(T, 2), 5*6*pow((t-s)/T, 4) /pow(T, 2), 6*7*pow((t-s)/T, 5) /pow(T, 2);
    Matrix<double, 8, 1> coff_x, coff_y, coff_z;
    coff_x = Polynomial_coefficients[0].block<8, 1>(8 * t_index, 0);
    coff_y = Polynomial_coefficients[1].block<8, 1>(8 * t_index, 0);
    coff_z = Polynomial_coefficients[2].block<8, 1>(8 * t_index, 0);
    p(0) = poly_p * coff_x;
    p(1) = poly_p * coff_y;
    p(2) = poly_p * coff_z;
    v(0) = poly_v * coff_x;
    v(1) = poly_v * coff_y;
    v(2) = poly_v * coff_z;
    a(0) = poly_a * coff_x;
    a(1) = poly_a * coff_y;
    a(2) = poly_a * coff_z;
    pva.push_back(p);
    pva.push_back(v);
    pva.push_back(a);
    return pva;
}