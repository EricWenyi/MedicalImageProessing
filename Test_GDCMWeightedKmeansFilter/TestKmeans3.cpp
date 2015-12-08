
#include <iostream>
#include <vector>
#include <cmath>
#define NumberOfVector 16
#define R 1
using namespace std;
typedef struct  //Struct
{
  double x;  //����1
  double y;  //����2
}Point;
void main()
{
   vector<Point> vecPoint;   //���е�
   vector<Point> vecCenter;  //���ĵ�
   vector<vector<Point> > vecCluster;  //��1��2
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
   for(i=0;i<NumberOfVector;i++) //��ʼ��������
   {
      temp.x=tempM[i][0];
      temp.y=tempM[i][1];
      vecPoint.push_back(temp);
   }
   //��ʼ�����ĵ�
   temp.x=-960;
   temp.y=102;
   vecCenter.push_back(temp);
   temp.x=-946;
   temp.y=-8;
   vecCenter.push_back(temp);
   double distance; //����
   double mindistance;
   unsigned int flag;  //����id
   unsigned int round=1;  //�����ִ�;
   double errorSum1;  //���ƽ����
   double errorSum2;  //���ƽ����
   vector<Point> tempvec;
   //double distance1,distance2;
   for(i=0;i<2;i++)   //��ʼ��Cluster vector
   {
      vecCluster.push_back(tempvec);
   }
   do
   {
      for(i=0;i<vecCluster.size();i++)  //���ÿһ���ض�Ӧ������
      {
         vecCluster[i].clear();
      }
      for(i=0;i<NumberOfVector;i++)    //�����ݶ�����й���
      {
         mindistance=(sqrt(pow(vecPoint[i].x-vecCenter[0].x,2.0)+pow(vecPoint[i].y-vecCenter[0].y,2.0)))/R;
         flag=0;
         for(int k=1;k<2;k++)   //���������ıȽϾ���   
         {
            distance=sqrt(pow(vecPoint[i].x-vecCenter[k].x,2.0)+pow(vecPoint[i].y-vecCenter[k].y,2.0));
            if(distance<mindistance)  //��ôص����ĸ���
            {
               flag=k;
               mindistance=distance;
            }
         }
         vecCluster[flag].push_back(vecPoint[i]);  //���ൽ��Ӧ�Ĵ�
      }// end of for(i=0;i<NumberOfVector;i++)
      
      cout<<"-------------��"<<round<<"�λ��ֽ��:-------------"<<endl;
      for(i=0;i<vecCluster.size();i++)  //������ִεĸ������ڵ�Ԫ��
      {
         cout<<"��"<<i+1<<": ";
         for(int j=0;j<vecCluster[i].size();j++)
         {
            cout<<"("<<vecCluster[i][j].x<<","<<vecCluster[i][j].y<<") ";
         }
         cout<<"��ѡ���ĵ�:("<<vecCenter[i].x<<","<<vecCenter[i].y<<")";
         cout<<endl;
      }
      
      if(round==1)
      {
         //�����ʼ�Ĵ��ڱ��
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
         errorSum1=errorSum2;  //��¼��һ�εĴ��ڱ��
      }
      
      round++;  //�ִμ�1
      double sum_x,sum_y;
      vecCenter.clear();  //���ԭ���Ĵ����ļ���
      for(int k=0;k<2;k++)   //���¼����������
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
         vecCenter.push_back(temp);  //�����µĴ�����
      }
      errorSum2=0;  //�����µĴ��ڱ��

      
      for(int k=0;k<2;k++)
      {        
         for(i=0;i<vecCluster[k].size();i++)
         {
            errorSum2+=pow(vecCenter[k].x-vecCluster[k][i].x,2.0)+pow(vecCenter[k].y-vecCluster[k][i].y,2.0);
         }
      }
      
   }while(fabs(errorSum2-errorSum1)>0.01);   //���ڱ���Ƿ������ȶ�
    //}while(distance1||distance2||distance3);
    cout<<endl<<">>>�����ĵ��Ѳ������ڱ���������ȶ�,���ֽ���!!!"<<endl;
    cout<<"------------���վ�����:-------------"<<endl;
    for(i=0;i<vecCluster.size();i++)  //������յľ�����
    {
       cout<<"��"<<i+1<<": ";
       for(int j=0;j<vecCluster[i].size();j++)
       {
          cout<<"("<<vecCluster[i][j].x<<","<<vecCluster[i][j].y<<") ";
       }
       cout<<"��ѡ���ĵ�:("<<vecCenter[i].x<<","<<vecCenter[i].y<<")";
       cout<<endl;
    }

	system("pause");
}