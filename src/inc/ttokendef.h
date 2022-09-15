/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TDENGINE_TTOKENDEF_H
#define TDENGINE_TTOKENDEF_H

#define TK_ID                               1
#define TK_BOOL                             2
#define TK_TINYINT                          3
#define TK_SMALLINT                         4
#define TK_INTEGER                          5
#define TK_BIGINT                           6
#define TK_FLOAT                            7
#define TK_DOUBLE                           8
#define TK_STRING                           9
#define TK_TIMESTAMP                       10
#define TK_BINARY                          11
#define TK_NCHAR                           12
#define TK_JSON                            13
#define TK_OR                              14
#define TK_AND                             15
#define TK_NOT                             16
#define TK_EQ                              17
#define TK_NE                              18
#define TK_ISNULL                          19
#define TK_NOTNULL                         20
#define TK_IS                              21
#define TK_LIKE                            22
#define TK_MATCH                           23
#define TK_NMATCH                          24
#define TK_CONTAINS                        25
#define TK_GLOB                            26
#define TK_BETWEEN                         27
#define TK_IN                              28
#define TK_GT                              29
#define TK_GE                              30
#define TK_LT                              31
#define TK_LE                              32
#define TK_BITAND                          33
#define TK_BITOR                           34
#define TK_BITXOR                          35
#define TK_LSHIFT                          36
#define TK_RSHIFT                          37
#define TK_PLUS                            38
#define TK_MINUS                           39
#define TK_DIVIDE                          40
#define TK_TIMES                           41
#define TK_STAR                            42
#define TK_SLASH                           43
#define TK_REM                             44
#define TK_UMINUS                          45
#define TK_UPLUS                           46
#define TK_BITNOT                          47
#define TK_ARROW                           48
#define TK_SHOW                            49
#define TK_DATABASES                       50
#define TK_TOPICS                          51
#define TK_FUNCTIONS                       52
#define TK_MNODES                          53
#define TK_DNODES                          54
#define TK_ACCOUNTS                        55
#define TK_USERS                           56
#define TK_MODULES                         57
#define TK_QUERIES                         58
#define TK_CONNECTIONS                     59
#define TK_STREAMS                         60
#define TK_VARIABLES                       61
#define TK_SCORES                          62
#define TK_GRANTS                          63
#define TK_VNODES                          64
#define TK_DOT                             65
#define TK_CREATE                          66
#define TK_TABLE                           67
#define TK_STABLE                          68
#define TK_DATABASE                        69
#define TK_TABLES                          70
#define TK_STABLES                         71
#define TK_VGROUPS                         72
#define TK_DROP                            73
#define TK_TOPIC                           74
#define TK_FUNCTION                        75
#define TK_DNODE                           76
#define TK_USER                            77
#define TK_ACCOUNT                         78
#define TK_USE                             79
#define TK_DESCRIBE                        80
#define TK_DESC                            81
#define TK_ALTER                           82
#define TK_PASS                            83
#define TK_PRIVILEGE                       84
#define TK_TAGS                            85
#define TK_LOCAL                           86
#define TK_COMPACT                         87
#define TK_LP                              88
#define TK_RP                              89
#define TK_IF                              90
#define TK_EXISTS                          91
#define TK_AS                              92
#define TK_OUTPUTTYPE                      93
#define TK_AGGREGATE                       94
#define TK_BUFSIZE                         95
#define TK_PPS                             96
#define TK_TSERIES                         97
#define TK_DBS                             98
#define TK_STORAGE                         99
#define TK_QTIME                          100
#define TK_CONNS                          101
#define TK_STATE                          102
#define TK_COMMA                          103
#define TK_KEEP                           104
#define TK_CACHE                          105
#define TK_REPLICA                        106
#define TK_QUORUM                         107
#define TK_DAYS                           108
#define TK_MINROWS                        109
#define TK_MAXROWS                        110
#define TK_BLOCKS                         111
#define TK_CTIME                          112
#define TK_WAL                            113
#define TK_FSYNC                          114
#define TK_COMP                           115
#define TK_PRECISION                      116
#define TK_UPDATE                         117
#define TK_CACHELAST                      118
#define TK_PARTITIONS                     119
#define TK_UNSIGNED                       120
#define TK_USING                          121
#define TK_TO                             122
#define TK_SPLIT                          123
#define TK_NULL                           124
#define TK_NOW                            125
#define TK_VARIABLE                       126
#define TK_SELECT                         127
#define TK_UNION                          128
#define TK_ALL                            129
#define TK_DISTINCT                       130
#define TK_FROM                           131
#define TK_RANGE                          132
#define TK_INTERVAL                       133
#define TK_EVERY                          134
#define TK_SESSION                        135
#define TK_STATE_WINDOW                   136
#define TK_FILL                           137
#define TK_SLIDING                        138
#define TK_ORDER                          139
#define TK_BY                             140
#define TK_ASC                            141
#define TK_GROUP                          142
#define TK_HAVING                         143
#define TK_LIMIT                          144
#define TK_OFFSET                         145
#define TK_SLIMIT                         146
#define TK_SOFFSET                        147
#define TK_WHERE                          148
#define TK_TODAY                          149
#define TK_RESET                          150
#define TK_QUERY                          151
#define TK_SYNCDB                         152
#define TK_ADD                            153
#define TK_COLUMN                         154
#define TK_MODIFY                         155
#define TK_TAG                            156
#define TK_CHANGE                         157
#define TK_SET                            158
#define TK_KILL                           159
#define TK_CONNECTION                     160
#define TK_STREAM                         161
#define TK_COLON                          162
#define TK_DELETE                         163
#define TK_ABORT                          164
#define TK_AFTER                          165
#define TK_ATTACH                         166
#define TK_BEFORE                         167
#define TK_BEGIN                          168
#define TK_CASCADE                        169
#define TK_CLUSTER                        170
#define TK_CONFLICT                       171
#define TK_COPY                           172
#define TK_DEFERRED                       173
#define TK_DELIMITERS                     174
#define TK_DETACH                         175
#define TK_EACH                           176
#define TK_END                            177
#define TK_EXPLAIN                        178
#define TK_FAIL                           179
#define TK_FOR                            180
#define TK_IGNORE                         181
#define TK_IMMEDIATE                      182
#define TK_INITIALLY                      183
#define TK_INSTEAD                        184
#define TK_KEY                            185
#define TK_OF                             186
#define TK_RAISE                          187
#define TK_REPLACE                        188
#define TK_RESTRICT                       189
#define TK_ROW                            190
#define TK_STATEMENT                      191
#define TK_TRIGGER                        192
#define TK_VIEW                           193
#define TK_IPTOKEN                        194
#define TK_SEMI                           195
#define TK_NONE                           196
#define TK_PREV                           197
#define TK_LINEAR                         198
#define TK_IMPORT                         199
#define TK_TBNAME                         200
#define TK_JOIN                           201
#define TK_INSERT                         202
#define TK_INTO                           203
#define TK_VALUES                         204
#define TK_FILE                           205


#define TK_SPACE                          300
#define TK_COMMENT                        301
#define TK_ILLEGAL                        302
#define TK_HEX                            303   // hex number  0x123
#define TK_OCT                            304   // oct number
#define TK_BIN                            305   // bin format data 0b111
#define TK_QUESTION                       307   // denoting the placeholder of "?",when invoking statement bind query

#endif


