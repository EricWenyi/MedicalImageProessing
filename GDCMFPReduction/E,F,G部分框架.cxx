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
	vector<AnObject> Objects； //推进去的Objects应当按照Label排序

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


	void Volume_Calculation_3D(Objects, Contour_Area, vector<> Object_Volumn){
		//计算Volume，放入相应的vector
	}

	void SurfaceArea_Calculation_3D(Objects, Contour_Area, Contour_Perimeter, vector<> Object_SurfaceArea){
		//计算Surface Area
	}

	void GreyValue_Calculation_3d(Objects, ){
		//??????Grey Value怎么算 SSK怎么算 想一想
	}

	void Eccentricity_Calculation(Objects, ){
		//??????
	}

	
	
	
