public class PitchDetectionResult
{
    /**
     * The pitch in Hertz.
     */
    private float pitch;

    private float probability;

    private bool pitched;

    public PitchDetectionResult()
    {
        pitch = -1;
        probability = -1;
        pitched = false;
    }

    /**
     * A copy constructor. Since PitchDetectionResult objects are reused for performance reasons, creating a copy can be practical.
     * @param other
     */
    public PitchDetectionResult(PitchDetectionResult other)
    {
        this.pitch = other.pitch;
        this.probability = other.probability;
        this.pitched = other.pitched;
    }


    /**
     * @return The pitch in Hertz.
     */
    public float getPitch()
    {
        return pitch;
    }

    public void setPitch(float pitch)
    {
        this.pitch = pitch;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#clone()
     */
    public PitchDetectionResult clone()
    {
        return new PitchDetectionResult(this);
    }

    /**
     * @return A probability (noisiness, (a)periodicity, salience, voicedness or
     *         clarity measure) for the detected pitch. This is somewhat similar
     *         to the term voiced which is used in speech recognition. This
     *         probability should be calculated together with the pitch. The
     *         exact meaning of the value depends on the detector used.
     */
    public float getProbability()
    {
        return probability;
    }

    public void setProbability(float probability)
    {
        this.probability = probability;
    }

    /**
     * @return Whether the algorithm thinks the block of audio is pitched. Keep
     *         in mind that an algorithm can come up with a best guess for a
     *         pitch even when isPitched() is false.
     */
    public bool isPitched()
    {
        return pitched;
    }

    public void setPitched(bool pitched)
    {
        this.pitched = pitched;
    }
}