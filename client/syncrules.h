#ifndef QUICKSYNC_SYNCRULES_H
#define QUICKSYNC_SYNCRULES_H

extern QString defaultFile;

enum SyncRuleFlags_e
{
  e_NoFlags           = 0,
  e_Binary            = 1<<0,
  e_Executable        = 1<<1,
  e_BinaryExecutable  = e_Binary | e_Executable
};

enum SyncRuleOrigin
{
  e_UserRule = 0,
  e_ExcludeDir,
  e_IncludeDir,
  e_ExcludeExtension,
  e_IncludeExtension,
};

class SyncRule
{
public:
  SyncRule();
  SyncRule(const QString& pattern, bool exclude, SyncRuleFlags_e flags);
  SyncRule(const SyncRule& other);
  ~SyncRule();

  bool compile(QString& error);

  bool operator==(const SyncRule& other)
  {
    return fExclude == other.fExclude &&
           fPattern == other.fPattern &&
           fFlags == fFlags;
  }
  bool operator!=(const SyncRule& other)
  {
    return fExclude != other.fExclude ||
           fPattern != other.fPattern ||
           fFlags != fFlags;
  }

  bool fExclude;
  QString fPattern;
  SyncRuleFlags_e fFlags;
  SyncRuleOrigin fOrigin;
  pcre* fRegex;
  pcre_extra* fRegexExtra;
};

class SyncRules : public QXmlDefaultHandler
{
public:
  SyncRules(void);
  SyncRules(const SyncRules& other);
  ~SyncRules(void);

  void save();
  void clear();
  int createRule(const QString& pattern, bool exclude, int flags, QString& error);
  int appendRule(SyncRule* rule);

  const SyncRule* getSyncRule(int index) const;
  SyncRule* getSyncRule(int index);
  int getNrSyncRules() const { return m_Rules.size(); }

  bool CheckFile(const QString& match, SyncRuleFlags_e& flags) const;
  bool CheckFile(const QString& match, SyncRuleFlags_e& flags, int& ruleIndex) const;
  bool CheckFileAndPath(const QString& match, SyncRuleFlags_e& flags) const;

  bool exportRules(const QString& i_fileName);
  //bool importRules(const QString& i_fileName);

  bool loadRules();
  bool loadXmlRules(const QString& i_fileName);
  bool saveXmlRules(const QString& i_fileName);
  QString getRuleLocation() const { return m_RuleLocation; }

  bool operator==(const SyncRules& other) const;
  bool operator!=(const SyncRules& other) const;
  void operator=(const SyncRules& other);

protected:
  void load();

  bool processRoot(const QDomElement& element);
  bool processRule(const QDomElement& element);
  QString getRuleNodeText(const QDomElement& element, const QString& name);
  QString m_XmlLoadingError;

  QVector<SyncRule*> m_Rules;
  QString m_LoadErrors;
  QString m_RuleLocation;
};

#endif //QUICKSYNC_SYNCRULES_H
