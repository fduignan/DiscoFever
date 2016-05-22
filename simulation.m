% Octave test file for DiscoFever;
fs=32768;
BlockSize=512;
WordLength=12;
OutputBinCount=16;

ScaleFactor=(2^WordLength)*BlockSize/2;
t=0:1/fs:BlockSize/fs;
s=(2^WordLength)*(sin(2*pi*2048*t));
S=abs(fft(s))/(ScaleFactor/256);
OutputValues=zeros(1,OutputBinCount);
for Index=1:OutputBinCount
  FrequencyComponent = 0;
  for AverageIndex = 1:(BlockSize / OutputBinCount)
    FrequencyComponent = FrequencyComponent + S((Index-1)*(BlockSize / OutputBinCount)+AverageIndex);
  end
  OutputValues(Index)=FrequencyComponent;
end
plot(S,'.')