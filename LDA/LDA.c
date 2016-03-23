
int calcEivectorAndCenter(double** data,vector<int>& labels,Mat& eivector,vector<Mat>& cluster,int sizeA,int sizeB){
	Mat mat=Mat(sizeA,sizeB,CV_64FC1,data);
	
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


int classDetermination(double** data,Mat& eivector,vector<int>& classBelongings,vector<Mat>& cluster,int sizeA,int sizeB){
    vector<Mat> result(sizeA);
	for(int i=0;i<sizeA;i++){
		result[i]=Mat::zeros(1,1,mat.type());
		multiply(eivector.t(),data[i],result[i]);
	}

	//vector<int> classBelongings(sizeA);
	for(int i=0;i<sizeA;i++){
		if((result[i].at<double>(0)-cluster[0].at<double>(0)*(result[i].at<double>(0)-cluster[0].at<double>(0))<(result[i].at<double>(0)-cluster[1].at<double>(0)*(result[i].at<double>(0)-cluster[1].at<double>(0))
		classBelongings[i]=0;
	    else
		classBelongings[i]=1;
	}
	return 0;

}