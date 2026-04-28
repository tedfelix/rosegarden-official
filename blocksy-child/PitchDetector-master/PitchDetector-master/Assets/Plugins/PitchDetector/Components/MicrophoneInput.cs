using UnityEngine;

namespace FinerGames.PitchDetector
{
    public class MicrophoneInput : MonoBehaviour
    {
        public AudioSource Source;

        public string DeviceName;
        public bool IsRecording = true;

        public int SampleRate = 44100;
    }
}