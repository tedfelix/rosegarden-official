using System.Collections.Generic;
using UnityEngine;

namespace FinerGames.PitchDetector
{
    public class SineInput : MonoBehaviour
    {
        [Range(1, 20000)]  //Creates a slider in the inspector
        public float frequency1;

        [Range(1, 20000)]  //Creates a slider in the inspector
        public float frequency2;

        public float sampleRate = 44100;
        public float waveLengthInSeconds = 2.0f;

        AudioSource audioSource;
        int timeIndex = 0;

        //[SerializeField] PitchDetectorDemo demo;

        public List<float> buffer;

        void Awake()
        {
            audioSource = gameObject.AddComponent<AudioSource>();
            audioSource.playOnAwake = false;
            audioSource.spatialBlend = 0; //force 2D sound
            audioSource.Stop(); //avoids audiosource from starting to play automatically
        }

        //void OnEnable()
        //{
        //    audioSource.Play();
        //}

        //void OnDisable()
        //{
        //    audioSource.Stop();
        //}

        void Update()
        {
            if (Input.GetKeyDown(KeyCode.Space))
            {
                if (!audioSource.isPlaying)
                {
                    timeIndex = 0;  //resets timer before playing sound
                    audioSource.Play();
                }
                else
                {
                    audioSource.Stop();
                }
            }
        }

        void OnAudioFilterRead(float[] data, int channels)
        {
            for (int i = 0; i < data.Length; i += channels)
            {
                data[i] = CreateSine(timeIndex, frequency1, sampleRate);

                if (channels == 2)
                    data[i + 1] = CreateSine(timeIndex, frequency2, sampleRate);

                timeIndex++;

                //if timeIndex gets too big, reset it to 0
                if (timeIndex >= (sampleRate * waveLengthInSeconds))
                {
                    timeIndex = 0;
                    buffer.Clear();
                }

                buffer.Add(data[i]);
            }
        }

        //Creates a sinewave
        public float CreateSine(int timeIndex, float frequency, float sampleRate)
        {
            return Mathf.Sin(2 * Mathf.PI * timeIndex * frequency / sampleRate);
        }
    }
}
 