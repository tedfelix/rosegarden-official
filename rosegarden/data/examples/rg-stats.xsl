<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html"/>

<!-- eek, can't get this working yet 
<xsl:param name="name" select="no composition name"/>
-->

<!-- XSLT for extracting some stats from an RG4 file
     See rosegarden/scripts/produceRGStats.sh to turn your RG file
     into XML that can be parsed with this stylesheet -->

<xsl:template match="/">
<html>
  <head>
    <title>Rosegarden File Summary</title>
  </head>
  <body>

      <xsl:apply-templates select="rosegarden-data/composition"/>

      <br/>

      <xsl:apply-templates select="rosegarden-data/segment"/>

      <br/>

      <xsl:apply-templates select="rosegarden-data/studio"/>

  </body>
</html>
</xsl:template>

<!-- Composition -->

<xsl:template match="composition">

  <!-- get the filename from the parameter list -->

  <h1>
  Rosegarden Composition <!-- ("<xsl:attribute select="$name"/>") -->
  </h1>

  <p>
  Copyright: <xsl:value-of select="@copyright"/>
  </p>

  <h2>
  Tracks
  </h2>

  <ul>
    <xsl:apply-templates select="track"/>
  </ul>
</xsl:template>

<xsl:template match="track">
  <li>
    Track
    <xsl:value-of select="@id"/>
    <xsl:text> (</xsl:text>
    <xsl:value-of select="@label"/>
    <xsl:text>)</xsl:text>
  </li>
</xsl:template>


<!-- Segment -->

<xsl:template match="segment">
  <h3>
    Segment ("
    <xsl:value-of select="@label"/>")
    [Track
    <xsl:value-of select="@track"/>]
    <xsl:text> - </xsl:text>
    starts at
    <xsl:value-of select="@start"/>
  </h3>

  <p>

    <!-- <xsl:apply-templates select="event"/> -->

    Contains <xsl:value-of select="count(event)"/> events.

  </p>

</xsl:template>

<xsl:template match="event">
  Event
</xsl:template>


<!-- Studio -->

<xsl:template match="studio">
  <h1>
  Studio
  </h1>

  <xsl:apply-templates select="device"/>

</xsl:template>

<xsl:template match="device">
  <h2>
  Device (id   = <xsl:value-of select="@id"/>,
          type = <xsl:value-of select="@type"/>)
  </h2>

  <ul>
    <xsl:apply-templates select="instrument"/>
  </ul>

  <ul>
    <xsl:apply-templates select="bank"/>
  </ul>

</xsl:template>

<xsl:template match="instrument">

  <li>
  Instrument (id      = <xsl:value-of select="@id"/>,
              channel = <xsl:value-of select="@channel"/>,
              type    = <xsl:value-of select="@type"/>)
  </li>

</xsl:template>

<xsl:template match="bank">

  <li>
  Bank (name = <xsl:value-of select="@name"/>,
        msb  = <xsl:value-of select="@msb"/>,
        lsb  = <xsl:value-of select="@lsb"/>)
  </li>

</xsl:template>

</xsl:stylesheet>
