<?xml version="1.0" encoding="UTF-8"?>

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:xhtml="http://www.w3.org/1999/xhtml">

  <xsl:output method="html" version="1.0" encoding="UTF-8"/>

  <xsl:template match="node()|@*">
    <xsl:copy>
      <xsl:apply-templates select="node()|@*"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="xhtml:div[@id='toc']">
    <xsl:copy>
      <xsl:copy-of select="@*"/>
      <div xmlns="http://www.w3.org/1999/xhtml">
        <ul class="sectlevel1">
          <li><a href="index.html">Home</a></li>
          <li><a href="documentation.html">Documentation</a></li>
          <li><a href="installation.html">Installation</a></li>
        </ul>
      </div>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

</xsl:transform>
