#include <iostream>
#include <contrib\contrib.hpp>
#include <cxcore.hpp>



#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>  
#include <fstream>


using namespace cv;
using namespace std;


//type:CV_64FC1
//clacEivectorAndCenter(训练集，训练集每个元素的数据类型，标签，特征向量，投影中心，训练集的size);
int calcEivectorAndCenter(vector<double> data,int type,vector<int>& labels,Mat& eivector,vector<Mat>& cluster,int sizeA,int sizeB){
	Mat mat=Mat(sizeA,sizeB,CV_64FC1,&data[0]);//一维数组转二维mat
	cout << "M = "<< endl << " "  << mat << endl << endl;
	LDA lda=LDA(mat,labels);
	eivector=lda.eigenvectors().clone();
	cout<<"The eigenvector is:"<<endl;
	for(int i=0;i<eivector.rows;i++)
	{
		for(int j=0;j<eivector.cols;j++)
		{
			cout<<eivector.ptr<double>(i)[j]<<" ";
		}
		cout<<endl;
	}
	
	
	//针对两类分类问题，计算两个数据集的中心
	int classNum=2;
	vector<Mat> classmean(classNum);
	vector<int> setNum(classNum);

	for(int i=0;i<classNum;i++)
	{
		classmean[i]=Mat::zeros(1,mat.cols,mat.type());
		setNum[i]=0;
	}

	Mat instance;
	for(int i=0;i<mat.rows;i++)
	{
		instance=mat.row(i);
		if(labels[i]==0)
		{	
			add(classmean[0], instance, classmean[0]);
			setNum[0]++;
		}
		else if(labels[i]==1)
		{
			add(classmean[1], instance, classmean[1]);
			setNum[1]++;
		}
		else
		{}
	}
	for(int i=0;i<classNum;i++)
	{
		classmean[i].convertTo(classmean[i],CV_64FC1,1.0/static_cast<double>(setNum[i]));
	}
	type=mat.type();
	//vector<Mat> cluster(classNum);
	for(int i=0;i<classNum;i++)
	{
		cluster[i]=Mat::zeros(1,1,mat.type());
		multiply(eivector.t(),classmean[i],cluster[i]);
	}

	cout<<"The project cluster center is:"<<endl;
	for(int i=0;i<classNum;i++)
	{
		cout<<cluster[i].at<double>(0)<<endl;
	}

	return 0;
	
}

//classDetermination(预测集,元素精度,特征向量,归属,投影中心,尺寸)
int classDetermination(vector<double> data,int type,Mat& eivector,vector<int>& classBelongings,vector<Mat>& cluster,int sizeA,int sizeB){
    vector<Mat> result(sizeA);

	for(int i=0;i<sizeA;i++){
		result[i]=Mat::zeros(1,1,type);
		Mat dataArray=Mat(1,sizeB,CV_64FC1,&(data[i*sizeB]));
		/*for(int j=0;j<10;j++)
			printf("dataArry[%d]=%f|%f",j,dataArray.at<double>(j),data[i*sizeB+j]);*/
		multiply(eivector.t(),dataArray,result[i]);
		printf("result[%d]=%f\n",i,result[i].at<double>(0));
	}



	//vector<int> classBelongings(sizeA);
	for(int i=0;i<sizeA;i++){
		if((result[i].at<double>(0)-cluster[0].at<double>(0))*(result[i].at<double>(0)-cluster[0].at<double>(0))<(result[i].at<double>(0)-cluster[1].at<double>(0))*(result[i].at<double>(0)-cluster[1].at<double>(0)))
		classBelongings[i]=0;
	    else
		classBelongings[i]=1;
	}
	return 0;

}




int main(){
	//导入
	std::ifstream file("C:\\Users\\TIAN\\Documents\\Tencent Files\\449006518\\FileRecv\\features.txt");
	boost::archive::text_iarchive ia(file);
	std::vector<double> features;
	ia & BOOST_SERIALIZATION_NVP(features);
	////导入结束
	//for(int i=0;i<features.size();i++){
	//	
	//	std::cout<<features[i]<<std::endl;
	//}
    

	//double sampledata[6][3]={{0,1,1},{0,2,1},{2,4,3},{8,0,1},{8,2,1},{9,4,3}};
	int type=0;

	
	//labels
	vector<int>labels;
	for(int i=0;i<6;i++)
	{
		if(i<3)
		{
			labels.push_back(0);
		}
		else
		{
			labels.push_back(1);
		}
	}

	int sizeA=features[features.size()-2];
	int sizeB=features[features.size()-1];
	features.erase(features.begin()+(features.size()-2));
	features.erase(features.begin()+(features.size()-1));

	Mat eivector;
	vector<Mat> cluster(features.size());

	calcEivectorAndCenter(features,type,labels,eivector,cluster,sizeA,sizeB);
	//calcEivectorAndCenter(sampledata,type,labels,eivector,cluster,sizeA,sizeB);
	
///
	///测试，人造的数据，人造的数字，
	///人造了6个，前3个所有值都是1 后三个所有值都是10
	///
	///
	///

	
	vector<int> classBelongings(6);


	vector<double> vTestData;
	for(int i=0;i<60;i++){
		if(i<30){
			vTestData.push_back(1);
		}else{
			vTestData.push_back(10);
		}
	}

	//printf("\nvTestData:\n");
	//for(int i=0;i<60;i++){
	//	printf("%f",vTestData[i]);
	//}

	classDetermination(vTestData,type,eivector,classBelongings,cluster,6,sizeB);
	for(int i=0;i<6;i++){
		printf("|%d\t",classBelongings[i]);
	}
	return 0;
}