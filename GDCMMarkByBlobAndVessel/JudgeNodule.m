function Nodule = JudgeNodule(FBlob,FVessel)
    %FBlob = CalcBlob(Lambda1, Lambda2, Lambda3);
    %FVessel = CalcVessel(Lambda1, Lambda2, Lambda3);
    Nodule = zeros(size(FBlob));
    Nodule(find(FBlob>0.35 & FVessel<0.7)) = 1;
    i = dicomread('lung.dcm');
    i = i(:,:,:);
    i(find(Nodule>0)) = 2047;
    i = reshape(i,size(i,1),size(i,2),1,size(i,3));
    dicomwrite(i,'temp.dcm');