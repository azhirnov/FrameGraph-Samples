/*
	Wave functions
*/

// Wave
float SineWave (float samp, float freq, float sampleRate)	// --U`U--
{
	return Sin( Pi() * samp * freq / sampleRate );
}

float SawWave (float samp, float freq, float sampleRate)	//  --/|/|--
{
	return Fract( samp * freq / sampleRate ) * 2.0 - 1.0;
}

float TriangleWave (float samp, float freq, float sampleRate)	// --/\/\--
{
	float value = Fract( samp * freq / sampleRate );
	return Min( value, 1.0 - value ) * 4.0 - 1.0;
}

// Gain
float LinearGain (float samp, float value, float startTime, float endTime, float sampleRate)
{
	float start = startTime * sampleRate;
	float end   = endTime * sampleRate;
	return samp > start and samp < end ?
			value * (1.0 - (samp - start) / (end - start)) :
			0.0;
}

float ExpGain (float samp, float value, float startTime, float endTime, float sampleRate)
{
	float start = startTime * sampleRate;
	float end   = endTime * sampleRate;
	float power = 4.0;
	return samp > start and samp < end ?
			value * (1.0 - pow( power, (samp - start) / (end - start) ) / power) :
			0.0;
}
