
#include <iostream>
#include <vector>
#include <cmath>
#define NumberOfVector 16
#define R 1
using namespace std;
typedef struct  //Struct
{
  double x;  //属性1
  double y;  //属性2
}Point;
void main()
{
   vector<Point> vecPoint;   //所有点
   vector<Point> vecCenter;  //中心点
   vector<vector<Point> > vecCluster;  //簇1，2
   int i = 0;
   double tempM[NumberOfVector][2]={
	   -807,54,//1
	   -1078,66,//2
	   -805,56,//3
	   -903,262,//4
	   -1100,114,//5
	   -901,52,//6
	   -1003,-52,//7
	   -855,70,//8
	   -812,96,//9
	   -1026,116,//10
	   -943,84,//11
	   -889,178,//12
	   -1018,16,//13
	   -930,-70,//14
	   -1033,-44,//15
	   -912,150,//16
   };
   Point temp;
   for(i=0;i<NumberOfVector;i++) //初始化各个点
   {
      temp.x=tempM[i][0];
      temp.y=tempM[i][1];
      vecPoint.push_back(temp);
   }
   //初始化中心点
   temp.x=-960;
   temp.y=102;
   vecCenter.push_back(temp);
   temp.x=-946;
   temp.y=-8;
   vecCenter.push_back(temp);
   double distance; //距离
   double mindistance;
   unsigned int flag;  //簇类id
   unsigned int round=1;  //迭代轮次;
   double errorSum1;  //误差平方和
   double errorSum2;  //误差平方和
   vector<Point> tempvec;
   //double distance1,distance2;
   for(i=0;i<2;i++)   //初始化Cluster vector
   {
      vecCluster.push_back(tempvec);
   }
   do
   {
      for(i=0;i<vecCluster.size();i++)  //清空每一个簇对应的容器
      {
         vecCluster[i].clear();
      }
      for(i=0;i<NumberOfVector;i++)    //对数据对象进行归类
      {
         mindistance=(sqrt(pow(vecPoint[i].x-vecCenter[0].x,2.0)+pow(vecPoint[i].y-vecCenter[0].y,2.0)))/R;
         flag=0;
         for(int k=1;k<2;k++)   //与两个中心比较距离   
         {
            distance=sqrt(pow(vecPoint[i].x-vecCenter[k].x,2.0)+pow(vecPoint[i].y-vecCenter[k].y,2.0));
            if(distance<mindistance)  //离该簇的中心更近
            {
               flag=k;
               mindistance=distance;
            }
         }
         vecCluster[flag].push_back(vecPoint[i]);  //归类到相应的簇
      }// end of for(i=0;i<NumberOfVector;i++)
      
      cout<<"-------------第"<<round<<"次划分结果:-------------"<<endl;
      for(i=0;i<vecCluster.size();i++)  //输出该轮次的各个簇内的元素
      {
         cout<<"簇"<<i+1<<": ";
         for(int j=0;j<vecCluster[i].size();j++)
         {
            cout<<"("<<vecCluster[i][j].x<<","<<vecCluster[i][j].y<<") ";
         }
         cout<<"所选中心点:("<<vecCenter[i].x<<","<<vecCenter[i].y<<")";
         cout<<endl;
      }
      
      if(round==1)
      {
         //计算初始的簇内变差
         errorSum1=0;
         for(int k=0;k<2;k++)
         {
            for(i=0;i<vecCluster[k].size();i++)
            {
                errorSum1+=pow(vecCenter[k].x-vecCluster[k][i].x,2.0)+pow(vecCenter[k].y-vecCluster[k][i].y,2.0);
            }
         }
      }
      else
      {
         errorSum1=errorSum2;  //记录上一次的簇内变差
      }
      
      round++;  //轮次加1
      double sum_x,sum_y;
      vecCenter.clear();  //清空原来的簇中心集合
      for(int k=0;k<2;k++)   //重新计算簇类中心
      {
         sum_x=0;
         sum_y=0;
         for(i=0;i<vecCluster[k].size();i++)
         {
             sum_x+=vecCluster[k][i].x;
             sum_y+=vecCluster[k][i].y;
         }
         temp.x=sum_x/vecCluster[k].size();
         temp.y=sum_y/vecCluster[k].size();
         vecCenter.push_back(temp);  //加入新的簇中心
      }
      errorSum2=0;  //计算新的簇内变差

      
      for(int k=0;k<2;k++)
      {        
         for(i=0;i<vecCluster[k].size();i++)
         {
            errorSum2+=pow(vecCenter[k].x-vecCluster[k][i].x,2.0)+pow(vecCenter[k].y-vecCluster[k][i].y,2.0);
         }
      }
      
   }while(fabs(errorSum2-errorSum1)>0.01);   //簇内变差是否收敛稳定
    //}while(distance1||distance2||distance3);
    cout<<endl<<">>>各中心点已不变或簇内变差已收敛稳定,划分结束!!!"<<endl;
    cout<<"------------最终聚类结果:-------------"<<endl;
    for(i=0;i<vecCluster.size();i++)  //输出最终的聚类结果
    {
       cout<<"簇"<<i+1<<": ";
       for(int j=0;j<vecCluster[i].size();j++)
       {
          cout<<"("<<vecCluster[i][j].x<<","<<vecCluster[i][j].y<<") ";
       }
       cout<<"所选中心点:("<<vecCenter[i].x<<","<<vecCenter[i].y<<")";
       cout<<endl;
    }

	system("pause");
}