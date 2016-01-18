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

