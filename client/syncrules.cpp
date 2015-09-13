#include "PreCompile.h"
#include "syncrules.h"
#include "utils.h"

const QString RulesFileHeader("QuickSync Rules");
QString defaultFile(joinPath(GetDataDir(), "syncrules.xml"));

SyncRule::SyncRule() : 
  fExclude(false), 
  fFlags(e_NoFlags),
  fOrigin(e_UserRule),
  fRegex(NULL)
{
}

SyncRule::SyncRule(const QString& pattern, bool exclude, SyncRuleFlags_e flags) :
  fExclude(exclude),
  fPattern(pattern),
  fFlags(flags),
  fOrigin(e_UserRule),
  fRegex(NULL)
{
}

SyncRule::SyncRule(const SyncRule& other) : 
  fExclude(other.fExclude), 
  fPattern(other.fPattern), 
  fFlags(other.fFlags),
  fOrigin(other.fOrigin),
  fRegex(NULL)
{
}

SyncRule::~SyncRule()
{
  if(fRegex != NULL)
  {
    pcre_free(fRegex);
    fRegex = NULL;
  }
}


bool SyncRule::compile(QString& error)
{
  QByteArray pattern = fPattern.toLower().toUtf8();
  const char* cpattern = pattern.constData();
  int erroffset;
  const char *cerror;
  fRegex = pcre_compile(cpattern, 0, &cerror, &erroffset, NULL);

  if(fRegex != NULL)
  {
    fRegexExtra = pcre_study(fRegex, 0, &cerror);
    return true;
  }

  error = QString("%1 at col %2\n").arg(cerror).arg(erroffset);
  return false;
}

SyncRules::SyncRules(void)
{
}

SyncRules::SyncRules(const SyncRules& other)
{
  for(int i = 0; i < other.m_Rules.size(); ++i)
  {
    SyncRule* rule = new SyncRule(*other.m_Rules.at(i));
    rule->compile(m_LoadErrors);
    m_Rules.append(rule);
  }
}

SyncRules::~SyncRules(void)
{
  clear();
}

bool SyncRules::loadRules()
{
  if(!loadXmlRules(defaultFile))
  {
    load();
    return saveXmlRules(defaultFile);
  }
  return true;
}

void SyncRules::load()
{
  m_Rules.clear();

  QSettings settings;
  int size = settings.beginReadArray("IgnoreRules");
  for(int i=0;i<size;i++)
  {
    settings.setArrayIndex(i);
    QString error;
    QString pattern = settings.value("pattern").toString();
    bool exclusive = settings.value("exclusive").toBool();
    int flags = settings.value("flags").toInt();
    if(createRule(pattern,exclusive,flags,error) == -1)
      m_LoadErrors.append(error);
  }
  settings.endArray();
  m_RuleLocation = "Registry";
}

void SyncRules::save()
{
  QSettings settings;
  settings.beginWriteArray("IgnoreRules");
  for(int i=0;i<m_Rules.size();i++)
  {
    SyncRule *rule = m_Rules[i];
    settings.setArrayIndex(i);
    settings.setValue("exclusive", rule->fExclude);
    settings.setValue("pattern", rule->fPattern);
    settings.setValue("flags", (int)rule->fFlags);
  }
  settings.endArray();
}

void SyncRules::clear()
{
  for(int i=0;i<m_Rules.size(); i++)
  {
    SyncRule* rule = m_Rules.at(i);
    delete rule;
  }
  m_Rules.clear();
  m_LoadErrors.clear();
}

int SyncRules::createRule(const QString& pattern, bool exclude, int flags, QString& error)
{
  SyncRule* rule = new SyncRule(pattern, exclude, SyncRuleFlags_e(flags));

  if(rule->compile(error))
    return appendRule(rule);

  return -1;
}

int SyncRules::appendRule(SyncRule* rule)
{
  int pos = m_Rules.size();
  m_Rules.push_back(rule);
  return pos;
}


const SyncRule* SyncRules::getSyncRule(int index) const 
{
  if(index >= 0 && index < m_Rules.size())
  {
    return m_Rules[index];
  }

  return NULL;
}

SyncRule* SyncRules::getSyncRule(int index)
{
  if(index >= 0 && index < m_Rules.size())
  {
    return m_Rules[index];
  }

  return NULL;
}


//Loop through the rules, if we get a match we return with the result false/true depending on 
//if the rule is a exclude/include rule. If we dont match, we allow.
bool SyncRules::CheckFile(const QString& match, SyncRuleFlags_e& flags) const
{
  int ruleIndex = 0;
  return CheckFile(match, flags, ruleIndex);
}

//The flags argument is always set to e_NoFlags unless the function hits an include rule, in that case flags is set to the rules flags member
//The ruleIndex will be set to -1 if there are no rules defined. It will be set to the rule used to include or exclude the path or if no rules are hit
//it will return fRules.size() (the index behind the last element in the rulelist)
bool SyncRules::CheckFile(const QString& match, SyncRuleFlags_e& flags, int& ruleIndex) const
{
  flags = e_NoFlags;
  ruleIndex = -1;
  QByteArray str = match.toUtf8();
  const char* pstr = str.constData();
  for(int i=0;i<m_Rules.size();i++)
  {
    SyncRule *rule = m_Rules[i];
    int res = pcre_exec(rule->fRegex, rule->fRegexExtra, pstr, str.length(), 0,0,0,0);
    if(res >= 0)
    {

      ruleIndex = i;
      if(rule->fExclude)
      {
        //qDebug() << "[SyncRules.CheckFile] Excluding " << match;
        return false;
      }
      else
      {
        flags = rule->fFlags;
        //qDebug() << "[SyncRules.CheckFile] Including " << match;
        return true; 
      }
    }

  }
  if(!m_Rules.isEmpty())
    ruleIndex = m_Rules.size();

  //No rule to exclude, so we allow
  //qDebug() << "[SyncRules.CheckFile] Default Including " << match;
  return true;
}

// This function will check each step of the path before checking the result, this was done to prevent
// different result in full scan and in autoscan.
bool SyncRules::CheckFileAndPath(const QString& match, SyncRuleFlags_e& flags) const 
{
  //skip the ./ part that always should be present
  int part = match.indexOf('/', 2);
  while(part > 0)
  {
    if(CheckFile(match.left(part), flags) == false)
    {
      //part of the path is denied so we fail
      return false;
    }
    part = match.indexOf('/', part + 1);
  }
  //no part of the path was denied, check the full path and file
  return CheckFile(match, flags);
}

bool SyncRules::operator==(const SyncRules& other) const
{
  //if they are of different sizes they are not equal
  if(m_Rules.size() != other.m_Rules.size())
    return false;

  //here we know they have the same number of rules
  int rules = m_Rules.size();
  for(int i=0;i<rules;i++)
  {
    //while the rules match we continue the loop, if they are different we return false
    if(m_Rules[i] == other.m_Rules[i])
      continue;
    return false;
  }

  //the rulesets are identical
  return true;
}

bool SyncRules::operator!=(const SyncRules& other) const
{
  //if they are of different sizes they are not equal
  if(m_Rules.size() != other.m_Rules.size())
    return true;

  //here we know they have the same number of rules
  int rules = m_Rules.size();
  for(int i=0;i<rules;i++)
  {
    //while the rules match we continue the loop, if they are different we return true
    if(m_Rules[i] == other.m_Rules[i])
      continue;
    return true;
  }

  //the rulesets are identical
  return false;
}

void SyncRules::operator=(const SyncRules& other)
{
  //Assign another set of rules to this object, clear the current rules, copy the others into this and compile all the rules
  m_Rules.clear();
  SyncRule* rule;
  foreach(rule, other.m_Rules)
  {
    QString error;
    createRule(rule->fPattern, rule->fExclude, rule->fFlags, error);
  }
}

namespace XmlStringNames
{
  static QString strRoot("syncrules");
  static QString strRule("rule");
  static QString strExclude("exclude");
  static QString strRegexPattern("pcre");
  static QString strFlags("flags");
  static QString strOrigins("origins");
  static QString strFalse("false");
  static QString strTrue("true");
  static QString strNoFlags("none");
  static QString strBinaryFlag("binary");
  static QString strExecutableFlag("executable");
  static QString strBinExeFlag("binary,executable");
  static QString strUserRuleOrigin("UserRule");
  static QString strExcludeDirOrigin("ExcludeDir");
  static QString strIncludeDirOrigin("IncludeDir");
  static QString strExcludeExtensionOrigin("ExcludeExtension");
  static QString strIncludeExtensionOrigin("IncludeExtension");
}

bool SyncRules::loadXmlRules(const QString& i_fileName)
{
  QFile xmlFile(i_fileName);
  if(!xmlFile.open(QIODevice::ReadOnly))
  {
    return false;
  }
  clear();

  QDomDocument xmlDoc;
  QString error;
  if(!xmlDoc.setContent(&xmlFile))
  {
    return false;
  }

  return processRoot(xmlDoc.documentElement());
}

bool SyncRules::processRoot(const QDomElement& element)
{
  if(element.tagName() != "syncrules")
  {
    m_XmlLoadingError = QString("Root node was not <syncrules> but <%1>").arg(element.tagName());
    return false;
  }

  QDomNodeList rules = element.elementsByTagName(XmlStringNames::strRule);
  for(int i = 0; i < rules.size(); ++i)
  {
    if(!rules.at(i).isElement())
    {
      m_XmlLoadingError = "File contained a node called rule that is not an element";
      return false;
    }
    if(!processRule(rules.at(i).toElement()))
    {
      return false;
    }
  }
  return true;
}

bool SyncRules::processRule(const QDomElement& element)
{
  QString pcre = getRuleNodeText(element, XmlStringNames::strRegexPattern);
  if(pcre.isEmpty())
    return false;
  QString excludeStr = getRuleNodeText(element, XmlStringNames::strExclude);
  if(excludeStr.isEmpty())
    return false;
  QString flagsStr = getRuleNodeText(element, XmlStringNames::strFlags);
  if(flagsStr.isEmpty())
    return false;

  SyncRuleFlags_e flags = e_NoFlags;
  if(flagsStr == XmlStringNames::strNoFlags)
  {
    flags = e_NoFlags;
  }
  else if(flagsStr == XmlStringNames::strBinaryFlag)
  {
    flags = e_Binary;
  }
  else if(flagsStr == XmlStringNames::strExecutableFlag)
  {
    flags = e_Executable;
  }
  else if(flagsStr == XmlStringNames::strBinExeFlag)
  {
    flags = e_BinaryExecutable;
  }
  else
  {
    m_XmlLoadingError = "Minor error, unknown flags. Defaulting to none";
  }
  bool exclude = true;
  if(excludeStr == XmlStringNames::strTrue)
  {
    exclude = true;
  }
  else if(excludeStr == XmlStringNames::strFalse)
  {
    exclude = false;
  }
  SyncRule* rule = new SyncRule;
  rule->fPattern = pcre;
  rule->fFlags = flags;
  rule->fExclude = exclude;

  if(rule->compile(m_XmlLoadingError))
  {
    m_Rules.append(rule);
    return true;
  }
  return false;
}

QString SyncRules::getRuleNodeText(const QDomElement& element, const QString& name)
{
  QDomNodeList nodes = element.elementsByTagName(name);
  if(nodes.size() != 1)
  {
    m_XmlLoadingError = QString("Rule contained %1 <%2> tags, only 1 is allowed").arg(nodes.size()).arg(name);
    return QString();
  }
  QDomElement nodeElement = nodes.at(0).toElement();
  if(nodeElement.isNull())
  {
    m_XmlLoadingError = QString("Got a non-element of %1").arg(name);
    return QString();
  }
  return nodeElement.text();
}

bool SyncRules::saveXmlRules(const QString& i_fileName)
{
  QFile xmlFile(i_fileName);
  if(!xmlFile.open(QIODevice::ReadWrite|QIODevice::Truncate))
  {
    return false;
  }

  QXmlStreamWriter xmlWriter(&xmlFile);
  xmlWriter.setAutoFormatting(true);

  xmlWriter.writeStartDocument("1.0");
  xmlWriter.writeStartElement(XmlStringNames::strRoot);

  SyncRule* rule;
  foreach(rule, m_Rules)
  {
    xmlWriter.writeStartElement(XmlStringNames::strRule);
    xmlWriter.writeStartElement(XmlStringNames::strExclude);
    xmlWriter.writeCharacters(rule->fExclude ? XmlStringNames::strTrue : XmlStringNames::strFalse);
    xmlWriter.writeEndElement(); //end exclude
    xmlWriter.writeStartElement(XmlStringNames::strRegexPattern);
    xmlWriter.writeCharacters(rule->fPattern);
    xmlWriter.writeEndElement();//end pcre
    xmlWriter.writeStartElement(XmlStringNames::strFlags);
    switch(rule->fFlags)
    {
      case e_NoFlags: xmlWriter.writeCharacters(XmlStringNames::strNoFlags); break;
      case e_Binary: xmlWriter.writeCharacters(XmlStringNames::strBinaryFlag); break;
      case e_Executable: xmlWriter.writeCharacters(XmlStringNames::strExecutableFlag); break;
      case e_BinaryExecutable: xmlWriter.writeCharacters(XmlStringNames::strBinExeFlag); break;
    }
    xmlWriter.writeEndElement();//end flags
    xmlWriter.writeEndElement();//end rule
  }
  xmlWriter.writeEndElement(); //end syncrules
  xmlWriter.writeEndDocument(); //end document

  xmlFile.close();

  return true;
}

//End
