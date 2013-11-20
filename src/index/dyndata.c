/* dyndata.c - dynamic data
 *
 * kevinhwm@gmail.com
 */

int GId_init(GIDynData *_this)
{
    if (_this == NULL) { return -1; }
    memset(_this, 0, sizeof(CIDynData));
    return 0;
}

