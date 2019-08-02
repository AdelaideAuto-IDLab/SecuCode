%%this script plot the normalized PDF for the WISP chip

%% plot the pdf before masking
clear all;
close all;
clc
idxA = 0;
bitLine = [];
bitProb = zeros(1,2048*8);%2048 bytes 8 bits 10 chipss

for idxA = 0:99
        chip = [];%hold the ubit8 array


            path = "./WISP238/"+"TEST"+idxA+".bin";
            fs = fopen(path,'rb');
            chip = fread(fs,'ubit8');
            fclose(fs);
            A = [];%bit string
            for idx=1:length(chip)
                A = [A,bitget(chip(idx),8:-1:1)];
            end
            bitProb = bitProb + A;
            disp("WISP chip test"+idxA);
end
figure(7);
bitPdf = zeros(1,101);
x = 0:100;
for idx = 0:100
    bitPdf(idx+1) = sum(bitProb == idx);
end

idx=1;
pp=[];
data = NaN(45:9);
    for idxA = 0:0
        for idxB = idxA:99
            if(idxA == idxB)
                continue;
            end
            idx = idx+1;
            path = "./WISP238/TEST"+idxA+".bin";
            fs = fopen(path,'rb');
            chip = fread(fs,'ubit8');
            fclose(fs);
            A = [];
            for idx=1:length(chip)
                A = [A,bitget(chip(idx),8:-1:1)];
            end

            path = "./WISP238/TEST"+idxB+".bin";
            fs = fopen(path,'rb');
            chip = fread(fs,'ubit8');
            fclose(fs);
            B = [];
            for idx=1:length(chip)
                B = [B,bitget(chip(idx),8:-1:1)];
            end
            lenge = length(A);
            dist = sum( A ~= B );
            p = (dist/lenge);
            pp = [pp,p];
            disp("WISP238 TEST"+idxA+"-to-"+idxB+" Total length: "+lenge+", Hamming distance: "+dist+", percentage: "+p*100);


        end
    end
    data(1,:) = pp(1,:);

    %plot overall 10 chip histfit
figure(18);
histfit(data(:));
%histfit(data(ChipID+1,:));
title('intra-chip HD for WISP238');
xlabel("mean =" + mean(pp(:))+", std =" + std(pp(:)));
axis([0,0.25,0,100]);

figure();
%bitPdfN = bitPdf./max(bitPdf);% normallize to the max value?
%semilogy(x,bitPdfN,'r+');
semilogy(x,bitPdf,'r+');
title("WISP238 PDF");

figure(1);
 bitPdf = zeros(1,101);
 bitCdf = zeros(1,101);
 cdf = 0;
x = 0:100;
for idx = 0:100
    bitPdf(idx+1) = sum(bitProb == idx);
    cdf = cdf + bitPdf(idx+1);
    bitCdf(idx+1) = cdf;
end
bitPdfN = bitPdf./max(bitPdf(:));
bitCdf = bitCdf ./ max(bitCdf(:));
plot(x,bitCdf,'b');
ylim([0 1]);
title("WISP238 Chip CDF");
%% make mask
%this script calculate the intra chip Hamming distance.
clc;
idx=1;
pp=[];
hdmap = zeros(1,2048*8);
data = NaN(45:9);
    for idxA =0:99
            path = "./WISP238/"+"TEST"+idxA+".bin";
            fs = fopen(path,'rb');
            chip = fread(fs,'ubit8');
            fclose(fs);
            A = [];%bit string
            for idx=1:length(chip)
                A = [A,bitget(chip(idx),8:-1:1)];
            end

            hdmap = hdmap + double(A == 1);
            disp("TEST"+idxA);

    end
figure(2);
    subplot(1,5,1);
hdmap = reshape(hdmap,16,[])';
hdmap2 = hdmap;
[r, c] = size(hdmap);                        % Get the matrix size
imagesc((1:c)+0.5, (1:r)+0.5, hdmap);        % Plot the image
title("Bit probablity");
%colorbar();
colormap(gray);                              % Use a gray colormap
axis equal                                   % Make axes grid sizes equal
ylim([0,64]);

hdmap2(hdmap2 == max(max(hdmap2))) = 0;
figure(10);
subplot(1,3,2);
imagesc((1:c)+0.5, (1:r)+0.5, hdmap2);        % Plot the image
title("unstable bits");
%colorbar();
colormap(gray);                              % Use a gray colormap
axis equal                                   % Make axes grid sizes equal
ylim([0,64]);

mid = 50;

hdmap3 = hdmap - hdmap2;
hdmap3(hdmap2 ~= 0) = mid;
figure(10);
subplot(1,3,3);
imagesc((1:c)+0.5, (1:r)+0.5, hdmap3);        % Plot the image
title("stable bits");
%colorbar();
colormap(gray);                              % Use a gray colormap
axis equal                                   % Make axes grid sizes equal
ylim([0,64]);

stable = 2048;
hdmap4 = hdmap3;
for idxw = 1:length(hdmap3(:,1))%remove unstable bytes
    for idxb = 0:1
       for idxi = 1:8
          if(hdmap3(idxw,idxb*8+idxi) == mid)
              hdmap4(idxw,(idxb*8)+1:(idxb*8)+8) = mid;
              stable = stable -1;
              break;
          end
       end
    end
end

%hdmap4(hdmap4 == mid) = max(max(hdmap4))/2;
hdmap4(hdmap4 ~= mid) = 100; % white out the window of the mask
figure(2);
subplot(1,5,2);
imagesc((1:c)+0.5, (1:r)+0.5, hdmap4, [0 100]);        % Plot the image
title("selected mask");
%colorbar();
colormap(gray);                              % Use a gray colormap

disp("Within 2048 bytes "+ stable +" bytes are stable.")
axis equal                                   % Make axes grid sizes equal
ylim([0,64]);
%% enroll and test
%Run this script after memMapOverTempStable.m
mask = hdmap4;%make the same size
mask(hdmap4 == 50) = -1;% don't care bytes
mask(mask ~= -1) = 1;% do care bytes

%linemask = mask(:)';
linemask = reshape(mask',1,[]);
pp=[];
hdmap = zeros(1,2048*8);
hdmapMasked = [];

%enroll at 25C
idxC = 3;
vote25 = zeros(1,2048*8);%2048 bytes 8 bits;
for idxA = 0:99
        chip = [];%hold the ubit8 array


            path = "./WISP238/"+"TEST"+idxA+".bin";
            fs = fopen(path,'rb');
            chip = fread(fs,'ubit8');
            fclose(fs);
            A = [];%bit string
            for idx=1:length(chip)
                A = [A,bitget(chip(idx),8:-1:1)];
            end
            vote25 = vote25 + A;
            disp("Enroll"+idxA);
end

%line25 = vote25(:)';
line25 = reshape(vote25',1,[]);
vote25 = reshape(vote25,16,[])';
line25 = imbinarize(line25, idxA/2);
vote25bin = imbinarize(vote25, max(vote25(:))/2);
enroll = double(vote25bin);%double(reshape(vote25bin,16,[])');
enroll(hdmap4 == 50) = 0.5;
figure(2);
subplot(1,5,3);
imagesc((1:c)+0.5, (1:r)+0.5, enroll);        % Plot the image
title("Selected bytes");
%colorbar();
colormap(gray);                              % Use a gray colormap
axis equal                                   % Make axes grid sizes equal
ylim([0,64]);

line25Norm = line25(linemask ~= -1);
line25Norm(line25Norm > 0) = 1;
    endp = 199;
    for idxA = 100:endp
            idx = idx+1;
            path = "./WISP238/"+"TEST"+idxA+".bin";
            fs = fopen(path,'rb');
            chip = fread(fs,'ubit8');
            fclose(fs);
            A = [];
            for idx=1:length(chip)
                A = [A,bitget(chip(idx),8:-1:1)];
            end
            Amask = A(linemask ~= -1);
            if(size(hdmapMasked) == 0)
                hdmapMasked = Amask;
            end
            hdmapMasked = hdmapMasked+Amask;
            dist = sum( Amask ~= line25Norm );
            p = dist/length(Amask);
            pp = [pp,p];
            hdmap = hdmap + double(A == 1);
            disp("TEST"+idxA);

    end
figure();
histfit(pp(:));
title('Pre-selected byte at 25C, intra HD in operating range ');
axis([0,0.25,0,100]);
format('short');
xlabel("mean =" + mean(pp(:))+", std =" + std(pp(:)));

figure(7);
hold on;
 mskbitPdf = zeros(1,101);
x = 0:100;
for idx = 0:100
    mskbitPdf(idx+1) = sum(hdmapMasked(:) == idx);
end
% bitPdfN = mskbitPdf./max(mskbitPdf(:));
% semilogy(x,bitPdfN,'r+');
semilogy(x,mskbitPdf,'r+');
title("Masked Normalized PDF");

%%plot cdf after mask
figure(1);
 mskbitPdf = zeros(1,101);
 mskbitCdf = zeros(1,101);
 cdf = 0;
x = 0:100;
for idx = 0:100
    mskbitPdf(idx+1) = sum(hdmapMasked(:) == idx);
    cdf = cdf + mskbitPdf(idx+1);
    mskbitCdf(idx+1) = cdf;
end
bitPdfN = mskbitPdf./max(mskbitPdf(:));
mskbitCdf = mskbitCdf ./ max(mskbitCdf(:));
hold on;
plot(x,mskbitCdf,'r+');
ylim([0 1]);
title("Masked CDF");

%% this script do de-baising by get rid of HW biased bytes

hdmapdb = enroll;
idxx = 0;
maskdb = zeros(1024,16);
for idxw = 1:length(hdmapdb(:,1))%remove biased
    for idxb = 0:1
        if(hdmapdb(idxw,(8*idxb+1))~=0.5)
            hammWeight = sum(hdmapdb(idxw,(8*idxb+1):(8*idxb+8)));
            if(hammWeight<6 && hammWeight>2)
                idxx = idxx+1;
                disp(idxx+")row="+idxw+" byte="+idxb+" HW="+hammWeight);
                maskdb(idxw,(idxb*8)+1:(idxb*8)+8) = 1;
            else
                 hdmapdb(idxw,(idxb*8)+1:(idxb*8)+8) = 0.5;
                 maskdb(idxw,(idxb*8)+1:(idxb*8)+8) = -1;
            end
        else
            maskdb(idxw,(idxb*8)+1:(idxb*8)+8) = -1;
        end

    end
end

figure(2);
subplot(1,5,5);
hold on;
imagesc((1:c)+0.5, (1:r)+0.5, hdmapdb);        % Plot the image
set(gca,'YDir','reverse');
title("enrolled CRP");
%colorbar();
colormap(gray);                              % Use a gray colormap
axis equal                                   % Make axes grid sizes equal
ylim([0,64]);

figure(2);
subplot(1,5,4);
hold on;
imagesc((1:c)+0.5, (1:r)+0.5, maskdb);        % Plot the image
set(gca,'YDir','reverse');
title("Debiasing mask");
%colorbar();
colormap(gray);                              % Use a gray colormap
axis equal                                   % Make axes grid sizes equal
ylim([0,64]);
hdmapdbMasked = [];
%linemaskdb = maskdb(:)';    % make the mask into a singe line
linemaskdb = reshape(maskdb',1,[]);
line25db = line25(linemaskdb ~= -1);
line25db(line25db > 0) = 1;
    endp = 99;
    for idxA = 0:endp
            idx = idx+1;
            path = "./WISP238/"+"TEST"+idxA+".bin";
            fs = fopen(path,'rb');
            chip = fread(fs,'ubit8');
            fclose(fs);
            A = [];
            for idx=1:length(chip)
                A = [A,bitget(chip(idx),8:-1:1)];
            end
            Amask = A(linemaskdb ~= -1);
            if(size(hdmapdbMasked) == 0)
                hdmapdbMasked = Amask;
            end
            hdmapdbMasked = hdmapdbMasked+Amask;

            dist = sum( Amask ~= line25db );
            p = dist/length(Amask);
            pp = [pp,p];

            disp("final "+idxA);

    end

figure();
histfit(pp(:));
title('Debiased byte at 25C, intra HD in operating range ');
axis([0,0.25,0,100]);
format('short');
xlabel("mean =" + mean(pp(:))+", std =" + std(pp(:)));


figure(7);
hold on;
 mskdbPdf = zeros(1,101);
x = 0:100;
for idx = 0:100
    mskdbPdf(idx+1) = sum(hdmapdbMasked(:) == idx);
end
semilogy(x,mskdbPdf,'g');
title("Debiased Normalized PDF");

%%Run this script after WISPCdfCompareAIO25.m and WISPDeBias25.m to
%%generate C code to run on WISP
%%
SRAMbase = hex2dec('1c00');
SRAMmask = hdmapdb;
Skip = hex2dec('1ea9');
%Skip = hex2dec('1c00');

BlockNum = 0;
Blockadd = 0;
offset = 0;
WindowNum = 0;
bitNum = 0;

fid = fopen('./puf1.h','wt');
fid2 = fopen('./db1.h','wt');%database of enrolled data.

fprintf(fid, '//For WISP238 only\n');
fprintf(fid2, '//For WISP238 only\n');

fprintf(fid, '#ifndef PUF_H_\n');
fprintf(fid, '#define PUF_H_\n');
fprintf(fid, '#include <stdint.h>\n');
fprintf(fid, '\n');

fprintf(fid2, '#ifndef DB_H_\n');
fprintf(fid2, '#define DB_H_\n');
fprintf(fid2, '#include <stdint.h>\n');
fprintf(fid2, '\n');

fprintf(fid, 'void getPUF(uint8_t ri[], uint8_t ci){\n');
fprintf(fid, 'ci = ci %s BLOCKS;\n',"%");
fprintf(fid, '\tswitch(ci){\n');
fprintf(fid, '\tcase %d:{\n',BlockNum);
fprintf(fid, '\t\tuint8_t *sram = (uint8_t *)(0x%s);\n',dec2hex(Blockadd+SRAMbase));

fprintf(fid2, 'void getPUF_DB(uint8_t ri[], uint8_t ci){\n');
fprintf(fid2, 'ci = ci %s BLOCKS;\n',"%");
fprintf(fid2, '\tswitch(ci){\n');
fprintf(fid2, '\tcase %d:{\n',BlockNum);

disp("Block "+BlockNum+" :"+dec2hex(Blockadd+SRAMbase));
flagw = 0;
for idxw = 0:length(SRAMmask(:,1))-1%convert to C struct
    if(flagw == 1)
       break;
    end
    for idxb = 0:1
        if(SRAMmask(idxw+1,(8*idxb+1))~=0.5)
            offset = 2*idxw+idxb;%calculate the byte offset, 1 word = 2 addresses
            if(offset+SRAMbase < Skip)
               continue;
            end
            disp("Window "+WindowNum+" at: "+dec2hex(offset-Blockadd));
            fprintf(fid, '\t\tri[%d] = sram[0x%s];',WindowNum,dec2hex(offset-Blockadd)); % write the window offset relative to block header
            fprintf(fid, '// %d %d %d %d %d %d %d %d;[%s]\n',SRAMmask(idxw+1,(8*idxb+1)),SRAMmask(idxw+1,(8*idxb+2)),SRAMmask(idxw+1,(8*idxb+3)),SRAMmask(idxw+1,(8*idxb+4)),SRAMmask(idxw+1,(8*idxb+5)),SRAMmask(idxw+1,(8*idxb+6)),SRAMmask(idxw+1,(8*idxb+7)),SRAMmask(idxw+1,(8*idxb+8)),dec2hex(offset));
            for itk =1:8
                fprintf(fid2, '\t\tri[%d] = %d;\n',bitNum,SRAMmask(idxw+1,(8*idxb+itk)));
                bitNum = bitNum+1;
            end
            WindowNum = WindowNum + 1;
            if(WindowNum >= 32)
               WindowNum = 0;
               Blockadd = offset;
               BlockNum = BlockNum + 1;% new block
               disp("Block "+BlockNum+":"+dec2hex(Blockadd+SRAMbase));
               fprintf(fid, '\t\tbreak;\n}\n');
               fprintf(fid2, '\t\tbreak;\n}\n');
               if(Blockadd+SRAMbase >= hex2dec('2300'))
                   flagw = 1;
                break;
               end
               bitNum = 0;
               fprintf(fid, '\tcase %d:{\n',BlockNum);
               fprintf(fid, '\t\tuint8_t *sram = (uint8_t *)(0x%s);\n',dec2hex(Blockadd+SRAMbase));
               fprintf(fid2, '\tcase %d:{\n',BlockNum);
            end
        end
    end
end
fprintf(fid, '\t}\n');
fprintf(fid2, '\t}\n');

fprintf(fid, '}\n');
fprintf(fid, '#define BLOCKS %d\n',BlockNum-1);
fprintf(fid, '#endif /* PUF_H_ */\n');

fprintf(fid2, '}\n');
fprintf(fid2, '#define BLOCKS %d\n',BlockNum-1);
fprintf(fid2, '#endif /* DB_H_ */\n');

fclose(fid);
fclose(fid2);