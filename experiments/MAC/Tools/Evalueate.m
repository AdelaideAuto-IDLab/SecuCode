
% clc;


DT = [];
VB = [];
Total = 0;
for idx = 0:99

%     direct = ".¡£/Data/Short/20ms/60cm/";
    direct = "../Data/Long/20ms/40cm/";
    scope = "scope_"+idx+"_";

    path = direct+scope;
    ch1Path = path + "1.csv";
    ch3Path = path + "3.csv";
    CH1 = csvread(ch1Path,2,0);
    CH3 = csvread(ch3Path,2,0);

    Time = CH1(:,1);
    Vout = CH1(:,2);
    Vbac = CH3(:,2);

    i2_0 = find(Vout>2,1);% where is 2V trigure fired
    i1_7m = find(Vout<1.7);% where is 2V trigure fired
    i1_7 = i1_7m(find(i1_7m>i2_0,1));

    ib = find(Vbac>1,1);
    DeltaT = Time(ib) - Time(i2_0);
    ValidBack = length(ib);
    
    if(ValidBack)
        DT = [DT,DeltaT];
        VB = [VB,ValidBack];
    else
        DeltaT = 0;
    end
    Total = Total+1;

end

mean(DT)
std(DT)
length(VB)/Total
