int repairContour(){
statusBegin=findStatusBegin();
if(statusBegin==-1){
	int newContourBeginI=findNewContourBegin();
	repairContour(newContourBeginI);//new contour
}
if(repairedByStatus[i].contour==currentContour){
		statusEnd=findStatusEnd();
		delAndDrawLine();
		int newContourBeginI=findNewContourBegin();
		repairContour(newContourBeginI);//new contour
}else{
	repairContour();//new contour
}
}

int findStatusBegin(Vector<APoint> repairedByStatus,int i){
	int currentContour=repairedByStatus[i].contour;
	while(repairedByStatus[i].contour==currentContour&&(!repairedByStatus[i].status))i++;
	if(repairedByStatus[i].contour!=currentContour)
		return -1;
	else
		return i;
}

int findStatusEnd(Vector<APoint> repairedByStatus,int i){
	int currentContour=repairedByStatus[i].contour;
	while(repairedByStatus[i].contour==currentContour&&(repairedByStatus[i].status))i++;
	return i-1;
}

int findNewContourBegin(Vector<APoint> repairedByStatus,int i){
	int currentContour=repairedByStatus[i].contour;
	while(repairedByStatus[i].contour==currentContour)i++;
	return i;
}

int delAndDrawLine(repairedByStatus,int i,int statusBegin,int statusEnd){
	int currentNode=0;
	while((contour[i][currentNode].x!=repairedByStatus[statusBegin].x)&&(contour[i][currentNode].y!=repairedByStatus.[statusBegin].y))
		currentNode++;

	while((contour[i][currentNode].x!=repairedByStatus[statusEnd].x)&&(contour[i][currentNode].y!=repairedByStatus[statusEnd]))
		NodeDelete();
	int Rx=repairedByStatus[statusEnd].x-repairedByStatus[statusBegin].x;
	int Ry=repairedByStatus[statusEnd].y-repairedByStatus[statusBegin].y;
	//TODO assert the node of statusEnd is known, then we have four possible related possition cases.
	if(Rx>0&&Ry>0)
	for(int x=1;x<=Rx;x++){
		//first one , normal one.
		int y=Ry/Rx * x;
		AddNewNode();
	}
	if(Rx<0&&Ry>0)
		for(int x=-1;x>=Rx;x--){
		//first one , normal one.
		int y=Ry/Rx * x;
		AddNewNode();
	}
	if(Rx<0&&Ry<0)
		for(int x=-1;x>=Rx;x--){
		//first one , normal one.
		int y=Ry/Rx * x;
		AddNewNode();
	}
	if(Rx>0&&Ry<0)
	for(int x=1;x<=Rx;x++){
		//first one , normal one.
		int y=Ry/Rx * x;
		AddNewNode();
	}
}

AddNewNode(int possition,int i,int x,int y){
	contours[i].insert(contours[i].begin()+possition,newNode);
}

NodeDelete(int possition,int i,int x,int y){
	contours[i].delete(contours[i].begin()+possition);
}
