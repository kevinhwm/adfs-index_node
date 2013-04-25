/* 
 * namespace.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

AIRecord * parse(const char *ori_record)
{
    if (ori_record == NULL)
	return NULL;

    char * rest = ori_record;
    char * cur_dollar = NULL;
    char * next_dollar = NULL;
    AIRecord *pr = malloc(sizeof(AIRecord));

    cur_dollar = strstr(rest, "$");
    while ( next_dollar = strstr(cur_dollar+1, "$") ) 
    {
	char * record = malloc(next_dollar - cur_dollar + 1);

	strncpy(pr->uuid, rest, cur_dollar - rest);
    }

    return pr;
}

