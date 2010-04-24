/*
 * Copyright 2010 Michał Lipski <tallica@o2.pl>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

static const char *_guess_pl(const unsigned char *ptr, int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        if (ptr[i] == 0xB9 || //ą
            ptr[i] == 0xE6 || //ć
            ptr[i] == 0xEA || //ę
            ptr[i] == 0xB3 || //ł
            ptr[i] == 0xF1 || //ń
            ptr[i] == 0xF3 || //ó
            ptr[i] == 0x9C || //ś
            ptr[i] == 0x9F || //ź
            ptr[i] == 0xBF || //ż
            ptr[i] == 0xA5 || //Ą
            ptr[i] == 0xC6 || //Ć
            ptr[i] == 0xCA || //Ę
            ptr[i] == 0xA3 || //Ł
            ptr[i] == 0xD1 || //Ń
            ptr[i] == 0xD3 || //Ó
            ptr[i] == 0x8C || //Ś
            ptr[i] == 0x8F || //Ź
            ptr[i] == 0xAF)   //Ż

            return "CP1250";
    }

    return "ISO-8859-2";
}

const char *guess_pl(const char *ptr, int size)
{
    return _guess_pl((const unsigned char *) ptr, size);
}
