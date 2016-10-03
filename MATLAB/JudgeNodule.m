function JudgeNodule(matFile, dcmFile)
	load(matFile);
	i = dicomread(dcmFile);
	i(find(FBlob > 0.5 & FVessel > 0 & FVessel < 0.7)) = 2047;
    dicomwrite(i,'mark.dcm');