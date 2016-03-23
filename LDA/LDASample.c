// LDA.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <contrib\contrib.hpp>
#include <cxcore.hpp>
using namespace cv;
using namespace std;

int main(void)
{
	//sampledata
	double sampledata[6][2]={{0,1},{0,2},{2,4},{8,0},{8,2},{9,4}};
	Mat mat=Mat(6,2,CV_64FC1,sampledata);
	//labels
	vector<int>labels;
	for(int i=0;i<mat.rows;i++)
	{
		if(i<mat.rows/2)
		{
			labels.push_back(0);
		}
		else
		{
			labels.push_back(1);
		}
	}

	//do LDA
	LDA lda=LDA(mat,labels);
	//get the eigenvector
	Mat eivector=lda.eigenvectors().clone();

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

	vector<Mat> cluster(classNum);
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
	
	system("pause");
	return 0;
}
