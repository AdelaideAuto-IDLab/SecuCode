clear all;
close all;
clc;
OVHEADidx = 1;
Sleep = ["0ms","10ms","20ms","30ms"];
Distance = ["20cm","40cm","60cm","80cm"]; 
for idx = 0:99 %transverse the whole folder
    path = "./BCH31/"+Sleep(4)+"/"+Distance(4)+"/scope_"+idx+"_";
    CH2 = csvread(path+"2.csv",2,0);
    CH1 = csvread(path+"1.csv",2,0);
    hold on;
    plot(CH2);
    plot(CH1);
    
    idx0 = find(CH1(:,2) > 1.8, 1, 'first');
    if(isempty(idx0))
        continue;
     %T0 = -1;
    else
       T0 = CH1(idx0,1); 
    end
    idxb = find(CH2(:,2) > 1.5, 1, 'first');
    if(isempty(idxb))
        continue;
        %Tb = -1;
    else
       Tb = CH2(idxb,1); 
    end

    TD = Tb-T0;
    OVERHEAD(OVHEADidx,1) = TD;
    OVHEADidx = OVHEADidx+1;
end
MTD = mean(OVERHEAD(:,1));
MSR = length(OVERHEAD(:,1))/(idx+1);
