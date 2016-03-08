Part E:

	struct Acontour{
		int Label;
		int slice_index;//在哪一层
		int index;//序号
		vector<Point> contour;//把之前的contours输入进去 ex.  Acontour C = new Acontour();  C.contour = contours[1]; 
	};

	struct AnObject{
		int Label;
		vector<Acontour> Object_Contours;//按照contour所在张数序号，排序
	};

	vector<vector<Acontour>> Contours;//第一层：所在的slice 第二层：contour对应的序号（不要与原来的openCV里的contours变量搞混）
	vector<AnObject> Objects； //推进去的Objects应当按照Label排5序

	//注意 Feature使用vector<AnObject> Objects 作为参数，计算出的数据应该是vector<vector<>>结构(对每一个contour都要求的feature) 
	//或者是 vector<>结构（只要求一个nodule中最大值）
	//对于vector<vector<>>结构， 第一层的序号是label，第二层的序号是张数的序号
	//对于vector<>结构，序号就是label


	//以下是函数的申明
	void Major_Minor_Axis_Calculation(vector<AnObject> Objects, vector<vector<>> major_Axis, vector<vector<>> minor_Axis, vector<vector<>> Axis_Ratio){
		//计算长短轴，将获得的数据存放在相应的vector<vector<>>里面
	}
	

	void Major_Minor_Axis_Delete(vector<vector<>> Axis_Ratio, vector<int> LaeblToDelete){
		//根据ratio,将需要删除的label push_back到LabelToDelete里面，记得要查重。
	}

	void BoundingBox_Calculation(vector<AnObject> Objects, vector<vector<>> BoundingBox_Area, 
		vector<vector<>> Contour_Area, vector<vector<>> BoundingBox_Height, vector<vector<>> BoundingBox_Width){
		//计算boundingbox的面积，nodule的面积，boundingbox的长宽。
	}
	
	void BoundingBox_Ratio_Delete(BoundingBox_Area, Contour_Area, vector<int> LaeblToDelete){
		//根据ratio,将需要删除的label push_back到LabelToDelete里面，记得要查重。
	}

	void BoundingBox_Size_Delete_3D(vector<AnObject> Objects, BoundingBox_Height, BoundingBox_Width, vector<int> LaeblToDelete){
		//根据BoundingBox的大小,将需要删除的label push_back到LabelToDelete里面，记得要查重。
	}

	void Maxium_Circularity_calculation_3D(vector<AnObject> Objects,vector<vector<>> Contour_Area, vector<vector<>> Contour_Perimeter,vector<> circularity, vector<> compactness, vector<int> LaeblToDelete){
		//???compactness怎么算
		//先计算出所有contour的perimeter，然后计算出最大的circularity和compactness
	}

	void Circularity_Delete_3D(vector<> circularity, vector<int> LaeblToDelete){
		//根据circularity,将需要删除的label push_back到LabelToDelete里面，记得要查重。
	}

//F部分：


	void Volume_Calculation_3D(Objects, Contour_Area, vector<double> Object_Volumn, vector<int> Object_Voxel_Number){
		//计算Volume以及每个nodule对应的voxel个数，放入相应的vector
		double volume = 0.0;
		int Voxel_Number = 0;
		for(int i = 0; i < Objects.size(); i++){
			for(int j = 0; j < Objects[i].size(); j++){
				volume += Contour_Area[i][j] * slice_width;     //Area 需要再乘上每一张slice之间的距离才是体积
				Voxel_Number += Contour_Area[i][j] / pixel_area; //Area 除以像素点面积就是每一个contour所包含的像素点
			}
			Object_Volumn.push_back(volume);
			Object_Voxel_Number.push_back(Voxel_Number);

		}
	}

	void SurfaceArea_Calculation_3D(Objects, Contour_Area, Contour_Perimeter, vector<> Object_SurfaceArea){
		double SurfaceArea = 0.0;

		for(int i = 0; i < Objects.size(); i++){
			for(int j = 0; j < Objects[i].Object_Contours.size(); j++){
				if(j = 0 || j = Objects[i].Object_Contours.size() - 1)
					SurfaceArea += Contour_Area[i][j] * slice_width;
				else
					SurfaceArea += Contour_Perimeter[i][j] * slice_width;

			}
		}
	}

	void GreyValue_Calculation_3d(Objects, vector<vector<>> Object_GrayValue){
		

	}

	void Eccentricity_Calculation(Objects, ){
		//??????
	}

	
	
	
