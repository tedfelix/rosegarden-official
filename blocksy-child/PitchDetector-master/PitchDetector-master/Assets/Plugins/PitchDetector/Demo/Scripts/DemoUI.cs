using System;
using System.Collections;
using Pitch;
using UnityEngine;
using UnityEngine.UI;

namespace FinerGames.PitchDetector.Demo
{
    public class DemoUI : MonoBehaviour
    {
        [SerializeField] Text detectorPitch;
        [SerializeField] Text detectorUnits;
        [SerializeField] Text detectorMidiNote;
        [SerializeField] Text detectorMidiCents;
        [SerializeField] Text detectorError;
        
        [SerializeField] PitchDetector detector;

        [SerializeField] float tickInterval = 0.01f;

        //void OnEnable()
        //{
        //    UpdateDisplay();
        //    StartCoroutine(Tick());
        //}

        //void OnDisable()
        //{
        //    StopCoroutine(Tick());
        //}

        //IEnumerator Tick()
        //{
        //    while (true)
        //    {
        //        yield return new WaitForSeconds(tickInterval);
        //        OnTick(null, null);
        //    }
        //}

        //void OnTick(object sender, EventArgs e)
        //{
        //    UpdateDisplay();
        //}

        void Update()
        {
            UpdateDisplay();
        }

        void UpdateDisplay()
        {
            // Show the detector results
            if (detector.Pitch > 1.0f)
            {
                if (detector.Pitch >= 1000.0f)
                {
                    detectorPitch.text = (detector.Pitch / 1000.0f).ToString("F4");
                    detectorUnits.text = "kHz";
                }
                else
                {
                    detectorPitch.text = detector.Pitch.ToString("F3");
                    detectorUnits.text = "Hz";
                }

                detectorMidiNote.text = PitchDsp.GetNoteName(detector.MidiNote, true, true);
                detectorMidiCents.text = detector.MidiCents.ToString();

                //var diffPercent = 100.0 - (100.0f * this.GeneratorPitch / curPitchRecord.Pitch);

                //if (diffPercent >= 0.0f)
                //    detectorError.text = "+" + diffPercent.ToString("F3");
                //else
                    //detectorError.text = diffPercent.ToString("F3");
            }
            else
            {
                detectorPitch.text = "--";
                detectorUnits.text = "Hz";
                detectorError.text = "--";
                detectorMidiNote.text = "--";
                detectorMidiCents.text = "--";
            }
        }
    }
}
