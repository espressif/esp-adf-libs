\
\
\
\
\
\

from __future__ import annotations

import zlib

import numpy as np

_cfomrienr = 0x00
_yjexhmm = 0x01
_aolftdpb = 0x02
_usvojwmv = 0x03
_ljhzfwyhc = 0x04
_qpkxojtc = 0x05
_qnfyke = 0x06
_wltpszoc = 0x07
_nbvcyrszk = 0x08
_ldrwcsb = 0x12
_pvrnyko = 0x13
_ipwvcb = 0x14

_yvzrmmmmd = 1
_ewqtuvxb = 2
_ijaenrl = 4
_umbgcgofd = 8
_bhbrejner = 0x01
_hcrnbs = 0x02
_emubcrdl = 0x04
_qgbcnn = 0x08
_huusbs = 0x10


def _qhwqirgoe(_mcpvjybtu: bytearray, _abauvt: int) -> None:
    _mcpvjybtu.append(_abauvt & 255)
    _mcpvjybtu.append((_abauvt >> 8) & 255)


def _dmokcvhnc(_mcpvjybtu: bytearray, _fgjujq: int, _abauvt: int) -> None:
    _mcpvjybtu[_fgjujq] = _abauvt & 255
    _mcpvjybtu[_fgjujq + 1] = (_abauvt >> 8) & 255
    _mcpvjybtu[_fgjujq + 2] = (_abauvt >> 16) & 255
    _mcpvjybtu[_fgjujq + 3] = (_abauvt >> 24) & 255


def _eieohxdm(_mcpvjybtu: bytearray, _abauvt: int) -> None:
    while _abauvt >= 0x80:
        _mcpvjybtu.append((_abauvt & 0x7F) | 0x80)
        _abauvt >>= 7
    _mcpvjybtu.append(_abauvt)


def _fvhfwnqmk(_aburltus, _bdapholbj: int) -> tuple[int, int]:
    _abauvt = _ogfeyczzu = 0
    while True:
        _jtbvmow = _aburltus[_bdapholbj]
        _bdapholbj += 1
        _abauvt |= (_jtbvmow & 0x7F) << _ogfeyczzu
        _ogfeyczzu += 7
        if not (_jtbvmow & 0x80):
            return _abauvt, _bdapholbj


def _vlerreaaz(_dpwoymz: np.ndarray, _wnlvmhec: int) -> np.ndarray:

    _npwofh, _pmyzoizys = _dpwoymz.shape
    _bvuouyf = (_npwofh + _wnlvmhec - 1) // _wnlvmhec
    _geuanaavv = (_pmyzoizys + _wnlvmhec - 1) // _wnlvmhec
    if _npwofh == _bvuouyf * _wnlvmhec and _pmyzoizys == _geuanaavv * _wnlvmhec:
        _vfbnyd = _dpwoymz.reshape(_bvuouyf, _wnlvmhec, _geuanaavv, _wnlvmhec).transpose(0, 2, 1, 3)
    else:
        _lpwbrs = np.empty((_bvuouyf * _wnlvmhec, _geuanaavv * _wnlvmhec), _dpwoymz.dtype)
        _lpwbrs[:_npwofh, :_pmyzoizys] = _dpwoymz
        if _geuanaavv * _wnlvmhec > _pmyzoizys:
            _lpwbrs[:_npwofh, _pmyzoizys:] = _dpwoymz[:, _pmyzoizys - 1:_pmyzoizys]
        if _bvuouyf * _wnlvmhec > _npwofh:
            _lpwbrs[_npwofh:, :] = _lpwbrs[_npwofh - 1:_npwofh, :]
        _vfbnyd = _lpwbrs.reshape(_bvuouyf, _wnlvmhec, _geuanaavv, _wnlvmhec).transpose(0, 2, 1, 3)
    return np.ascontiguousarray(_vfbnyd.reshape(_bvuouyf * _geuanaavv, _wnlvmhec * _wnlvmhec))


def _qxengg(_uhjprvmdf: np.ndarray, _xxpizc: int, _gqtuquunt: int) -> np.ndarray:

    _oeiqpgsi, _tqxphfabb = _uhjprvmdf.shape
    _bwqunxw = np.zeros((_oeiqpgsi, _gqtuquunt * 8), np.uint8)
    _atrvge = _tqxphfabb * _xxpizc
    for _ibaaxq in range(_xxpizc):
        _bwqunxw[:, _ibaaxq:_atrvge:_xxpizc] = (_uhjprvmdf >> _ibaaxq) & 1
    return np.packbits(_bwqunxw, axis=1, bitorder="little")


def _tacgtmeu(_psqbdy: np.ndarray, _lezjrw: int) -> tuple[np.ndarray, np.ndarray]:
    _wgqcxovdr = np.all(_psqbdy == _psqbdy[:, :, :1], axis=2)
    _mldehxg = np.zeros(_wgqcxovdr.shape, np.bool_)
    _mldehxg[:, 1:] = np.all(_psqbdy[:, 1:] == _psqbdy[:, :-1], axis=2)
    _merqspwk = np.zeros(_wgqcxovdr.shape, np.bool_)
    if _psqbdy.shape[1] > _lezjrw:
        _merqspwk[:, _lezjrw:] = np.all(_psqbdy[:, _lezjrw:] == _psqbdy[:, :-_lezjrw], axis=2)
    _fvdgme = np.zeros(_wgqcxovdr.shape, np.bool_)
    if _psqbdy.shape[0] > 1:
        _fvdgme[1:] = np.all(_psqbdy[1:] == _psqbdy[:-1], axis=2)
    _bmboczbig = np.zeros(_wgqcxovdr.shape, np.uint8)
    _bmboczbig[_wgqcxovdr] |= _umbgcgofd
    _bmboczbig[_mldehxg] |= _ewqtuvxb
    _bmboczbig[_merqspwk] |= _ijaenrl
    _bmboczbig[_fvdgme] |= _yvzrmmmmd
    _inhepp = np.ascontiguousarray(_psqbdy[:, :, 0])
    return _bmboczbig, _inhepp


def _ltlpddpo(_klbgfmf: np.ndarray) -> np.ndarray:

    _tqxphfabb = _klbgfmf.shape[1]
    _dbgtsgc = np.arange(_tqxphfabb, dtype=np.int32)
    _nrqfmuho = np.where(~_klbgfmf, _dbgtsgc, np.int32(_tqxphfabb))
    _xgfxkgm = np.minimum.accumulate(_nrqfmuho[:, ::-1], axis=1)[:, ::-1]
    return np.where(_klbgfmf, _xgfxkgm - _dbgtsgc, 0).astype(np.int32, copy=False)


def _qorayc(_wgqcxovdr: np.ndarray, _krckhli: np.ndarray) -> np.ndarray:

    _tqxphfabb = _wgqcxovdr.shape[1]
    _dbgtsgc = np.arange(_tqxphfabb, dtype=np.int32)
    _uzmcstl = np.where(_wgqcxovdr, _krckhli.astype(np.int32, copy=False), -(_dbgtsgc + 1))
    _sclnyibid = np.empty(_wgqcxovdr.shape, np.bool_)
    _sclnyibid[:, 0] = True
    if _tqxphfabb > 1:
        _sclnyibid[:, 1:] = _uzmcstl[:, 1:] != _uzmcstl[:, :-1]
    _nrqfmuho = np.where(_sclnyibid, _dbgtsgc, np.int32(_tqxphfabb))
    _tyvbzqrtm = np.minimum.accumulate(_nrqfmuho[:, ::-1], axis=1)[:, ::-1]
    _xgfxkgm = np.empty(_wgqcxovdr.shape, np.int32)
    if _tqxphfabb > 1:
        _xgfxkgm[:, :-1] = _tyvbzqrtm[:, 1:]
    _xgfxkgm[:, -1] = _tqxphfabb
    return np.where(_wgqcxovdr, _xgfxkgm - _dbgtsgc, 0)


def _irnebxlo(_eeqvdxw: np.ndarray, _tqlfhkgi: np.ndarray, _yjjisi: np.ndarray, _ehvhaps: int,
              _wcsjfna: np.ndarray | None = None, _qldsorg: np.ndarray | None = None):

    _tqxphfabb, _sjrvfd = _eeqvdxw.shape
    _wcxbcs = np.sort(_eeqvdxw, axis=1)
    _sclnyibid = np.empty((_tqxphfabb, _sjrvfd), np.bool_)
    _sclnyibid[:, 0] = True
    if _sjrvfd > 1:
        _sclnyibid[:, 1:] = _wcxbcs[:, 1:] != _wcxbcs[:, :-1]
    _qjcjubx = _sclnyibid.sum(axis=1)
    _bnwyzps = np.cumsum(_sclnyibid, axis=1)
    _yxwwexyc = np.empty((_tqxphfabb, 8), np.uint16)
    _yxwwexyc[:, 0] = _wcxbcs[:, 0]
    _nnuwvl = np.arange(_tqxphfabb)
    for _njvjnbjx in range(1, 8):
        _notvhxr = _qjcjubx > _njvjnbjx
        _nrqfmuho = np.argmax(_bnwyzps == (_njvjnbjx + 1), axis=1)
        _yxwwexyc[:, _njvjnbjx] = np.where(_notvhxr, _wcxbcs[_nnuwvl, _nrqfmuho], _yxwwexyc[:, _njvjnbjx - 1])
    _uhjprvmdf = np.zeros((_tqxphfabb, _sjrvfd), np.uint8)
    for _njvjnbjx in range(1, 8):
        _uhjprvmdf[(_qjcjubx > _njvjnbjx)[:, None] & (_eeqvdxw == _yxwwexyc[:, _njvjnbjx:_njvjnbjx + 1])] = np.uint8(_njvjnbjx)

    _uhcsqyev = np.int32(1 << 30)
    _uwdnhzmw = np.int32(1 + _sjrvfd * _ehvhaps)
    _urwpovib = _qjcjubx <= 8
    _xxpizc = np.where(_qjcjubx <= 2, np.int16(1), np.where(_qjcjubx <= 4, np.int16(2), np.int16(3)))
    _tgpfsgbmq = np.where(_qjcjubx <= 2, np.int16(2), np.where(_qjcjubx <= 4, np.int16(4), np.int16(8)))
    _qijusvkx = (np.int32(_sjrvfd) * _xxpizc + 7) >> 3
    _ujpbref = np.where(_urwpovib, 1 + _tgpfsgbmq * _ehvhaps + _qijusvkx, _uhcsqyev).astype(np.int32)
    _qughqyac = np.where(_ujpbref < _uwdnhzmw, _ujpbref, _uwdnhzmw)

    _pvuncih = _eeqvdxw != _tqlfhkgi
    _ihztaarjo = _pvuncih.sum(axis=1, dtype=np.int32)
    _rtywytu = np.where(_yjjisi, 2 + _ihztaarjo * (1 + _ehvhaps), _uhcsqyev).astype(np.int32)
    _ulorqs = np.where(_yjjisi, 1 + ((_sjrvfd + 7) >> 3) + _ihztaarjo * _ehvhaps, _uhcsqyev).astype(np.int32)
    _lskqeo = (_rtywytu <= _qughqyac) & (_rtywytu <= _ulorqs)
    _irumxz = ~_lskqeo & (_ulorqs <= _qughqyac)
    _nkjjixxui = ~_lskqeo & ~_irumxz & (_ujpbref < _uwdnhzmw)
    _inpoatn = ~(_lskqeo | _irumxz | _nkjjixxui)

    _imlselky = 2 + _sjrvfd * (1 + _ehvhaps)
    _fduycpzwr = np.zeros((_tqxphfabb, _imlselky), np.uint8)
    _mtoruy = np.zeros(_tqxphfabb, np.uint16)
    _pulzuc(_fduycpzwr, _mtoruy, _eeqvdxw, _inpoatn, _ehvhaps)
    _ozxllchd(_fduycpzwr, _mtoruy, _eeqvdxw, _yxwwexyc, _uhjprvmdf, _xxpizc, _nkjjixxui, _ehvhaps)
    _pgkgptt(_fduycpzwr, _mtoruy, _eeqvdxw, _pvuncih, _ihztaarjo, _lskqeo, _irumxz, _ehvhaps)
    if _tqxphfabb and int(_mtoruy.min()) <= 0:
        raise RuntimeError("internal encoder missed a tile")
    return _fduycpzwr, _mtoruy


def _pulzuc(_fduycpzwr, _mtoruy, _eeqvdxw, _vlejmb, _ehvhaps: int) -> None:
    if not np.any(_vlejmb):
        return
    _tqxphfabb = _eeqvdxw.shape[1]
    _fduycpzwr[_vlejmb, 0] = _usvojwmv
    _nualgj = _eeqvdxw[_vlejmb]
    if _ehvhaps == 1:
        _fduycpzwr[_vlejmb, 1:1 + _tqxphfabb] = _nualgj.astype(np.uint8)
        _mtoruy[_vlejmb] = 1 + _tqxphfabb
        return
    _vfudonk = np.ascontiguousarray(_nualgj, dtype="<u2").view(np.uint8).reshape(-1, _tqxphfabb * 2)
    _fduycpzwr[_vlejmb, 1:1 + _tqxphfabb * 2] = _vfudonk
    _mtoruy[_vlejmb] = 1 + _tqxphfabb * 2


def _ozxllchd(_fduycpzwr, _mtoruy, _eeqvdxw, _yxwwexyc, _uhjprvmdf, _xxpizc, _nkjjixxui, _ehvhaps: int) -> None:
    for _hxyghxuo, _waabewu in ((1, _ljhzfwyhc), (2, _qpkxojtc), (3, _qnfyke)):
        _vlejmb = _nkjjixxui & (_xxpizc == _hxyghxuo)
        if not np.any(_vlejmb):
            continue
        _nnayzlug = 1 << _hxyghxuo
        _ohftxb = (_eeqvdxw.shape[1] * _hxyghxuo + 7) // 8
        _bwqunxw = _qxengg(_uhjprvmdf[_vlejmb], _hxyghxuo, _ohftxb)
        _fduycpzwr[_vlejmb, 0] = _waabewu
        _axnmnxq = _yxwwexyc[_vlejmb, :_nnayzlug]
        if _ehvhaps == 1:
            _fduycpzwr[_vlejmb, 1:1 + _nnayzlug] = _axnmnxq.astype(np.uint8)
            _fduycpzwr[_vlejmb, 1 + _nnayzlug:1 + _nnayzlug + _ohftxb] = _bwqunxw
            _mtoruy[_vlejmb] = 1 + _nnayzlug + _ohftxb
        else:
            _vfudonk = np.ascontiguousarray(_axnmnxq, dtype="<u2").view(np.uint8).reshape(-1, _nnayzlug * 2)
            _fduycpzwr[_vlejmb, 1:1 + _nnayzlug * 2] = _vfudonk
            _fduycpzwr[_vlejmb, 1 + _nnayzlug * 2:1 + _nnayzlug * 2 + _ohftxb] = _bwqunxw
            _mtoruy[_vlejmb] = 1 + _nnayzlug * 2 + _ohftxb


def _pgkgptt(_fduycpzwr, _mtoruy, _eeqvdxw, _pvuncih, _ihztaarjo, _lskqeo, _irumxz, _ehvhaps: int) -> None:
    _tqxphfabb = _eeqvdxw.shape[1]
    _ohftxb = (_tqxphfabb + 7) // 8
    if np.any(_irumxz):
        _zbmxrm = _pvuncih[_irumxz]
        _kgkwje = _eeqvdxw[_irumxz]
        _vkmfzzid = np.zeros((_zbmxrm.shape[0], _ohftxb), np.uint8)
        for _bdapholbj in range(_tqxphfabb):
            _vkmfzzid[:, _bdapholbj >> 3] |= _zbmxrm[:, _bdapholbj].astype(np.uint8) << (_bdapholbj & 7)
        _kqiqlgupz = np.zeros((_zbmxrm.shape[0], _fduycpzwr.shape[1]), np.uint8)
        _kqiqlgupz[:, 0] = _nbvcyrszk
        _kqiqlgupz[:, 1:1 + _ohftxb] = _vkmfzzid
        _ubiimmba, _mikybpi = np.nonzero(_zbmxrm)
        if len(_ubiimmba):
            _zegmzrft = np.cumsum(_zbmxrm, axis=1)[_ubiimmba, _mikybpi] - 1
            _ezdlufu = _kgkwje[_ubiimmba, _mikybpi]
            _wjboffi = 1 + _ohftxb
            if _ehvhaps == 1:
                _kqiqlgupz[_ubiimmba, _wjboffi + _zegmzrft] = _ezdlufu.astype(np.uint8)
            else:
                _kqiqlgupz[_ubiimmba, _wjboffi + _zegmzrft * 2] = _ezdlufu & 255
                _kqiqlgupz[_ubiimmba, _wjboffi + _zegmzrft * 2 + 1] = (_ezdlufu >> 8) & 255
        _fduycpzwr[_irumxz] = _kqiqlgupz
        _mtoruy[_irumxz] = (1 + _ohftxb + _ihztaarjo[_irumxz] * _ehvhaps).astype(np.uint16)
    if not np.any(_lskqeo):
        return
    _zbmxrm = _pvuncih[_lskqeo]
    _kgkwje = _eeqvdxw[_lskqeo]
    _kqiqlgupz = np.zeros((_zbmxrm.shape[0], _fduycpzwr.shape[1]), np.uint8)
    _kqiqlgupz[:, 0] = _wltpszoc
    _kqiqlgupz[:, 1] = _ihztaarjo[_lskqeo].astype(np.uint8)
    _ubiimmba, _mikybpi = np.nonzero(_zbmxrm)
    if len(_ubiimmba):
        _zegmzrft = np.cumsum(_zbmxrm, axis=1)[_ubiimmba, _mikybpi] - 1
        _kpyqrygj = 1 + _ehvhaps
        _kqiqlgupz[_ubiimmba, 2 + _zegmzrft * _kpyqrygj] = _mikybpi.astype(np.uint8)
        _ezdlufu = _kgkwje[_ubiimmba, _mikybpi]
        if _ehvhaps == 1:
            _kqiqlgupz[_ubiimmba, 3 + _zegmzrft * _kpyqrygj] = _ezdlufu.astype(np.uint8)
        else:
            _kqiqlgupz[_ubiimmba, 3 + _zegmzrft * _kpyqrygj] = _ezdlufu & 255
            _kqiqlgupz[_ubiimmba, 4 + _zegmzrft * _kpyqrygj] = (_ezdlufu >> 8) & 255
    _fduycpzwr[_lskqeo] = _kqiqlgupz
    _mtoruy[_lskqeo] = (2 + _ihztaarjo[_lskqeo] * (1 + _ehvhaps)).astype(np.uint16)


def _uhtzzygz(frames: np.ndarray, _wnlvmhec: int, _ehvhaps: int, _qisfjgwh: np.ndarray,
                  _mjxenlmkd: np.ndarray | None, _eljfqfc: int) -> bytearray:
    _zncbwpgl, _npwofh, _pmyzoizys = frames.shape
    _lezjrw = (_pmyzoizys + _wnlvmhec - 1) // _wnlvmhec
    _jfnbkans = (_npwofh + _wnlvmhec - 1) // _wnlvmhec
    _wbiurvnv = _lezjrw * _jfnbkans
    _tqxphfabb = _wnlvmhec * _wnlvmhec
    _uuaatbvx = _mjxenlmkd[frames] if _ehvhaps == 1 else frames
    _psqbdy = np.empty((_zncbwpgl, _wbiurvnv, _tqxphfabb), np.uint16)
    _qauzlqknx = np.empty((_zncbwpgl, _wbiurvnv, _tqxphfabb), np.uint16)
    for _rygmly in range(_zncbwpgl):
        _psqbdy[_rygmly] = _vlerreaaz(_uuaatbvx[_rygmly], _wnlvmhec)
        _qauzlqknx[_rygmly] = _vlerreaaz(frames[_rygmly], _wnlvmhec)
    _bmboczbig, _inhepp = _tacgtmeu(_psqbdy, _lezjrw)
    _udigmo = _ltlpddpo((_bmboczbig & _yvzrmmmmd) != 0)
    _ryvbvk = _ltlpddpo((_bmboczbig & _ewqtuvxb) != 0)
    _ibzwfn = _ltlpddpo((_bmboczbig & _ijaenrl) != 0)
    _frtxafkow = _qorayc((_bmboczbig & _umbgcgofd) != 0, _inhepp)
    _cwtwfuzht = []
    _ezdlufu = []
    _mikybpi = []
    _skzeql = []
    _npepfazx = [0]
    _xmganfndc, _mjjxtf, _xjilcm, _purxhel = _cwtwfuzht.append, _ezdlufu.append, _mikybpi.append, _skzeql.append
    for _rygmly in range(_zncbwpgl):
        _foowyish = memoryview(_udigmo[_rygmly])
        _cltyifk = memoryview(_ryvbvk[_rygmly])
        _qwhdfi = memoryview(_ibzwfn[_rygmly])
        _vjolnnvcf = memoryview(_frtxafkow[_rygmly])
        _vrzujg = memoryview(_inhepp[_rygmly])
        _wjboffi = _rygmly * _wbiurvnv
        _rpcndzmvr = _rygmly > 0
        _mjdabg = 0
        while _mjdabg < _wbiurvnv:
            _tdsehfyo = _foowyish[_mjdabg] if _rpcndzmvr else 0
            _mldehxg = _cltyifk[_mjdabg]
            _merqspwk = _qwhdfi[_mjdabg]
            _poewph = _vjolnnvcf[_mjdabg]
            _qughqyac = _tdsehfyo
            _jebpkbt = 0 if _tdsehfyo else -1
            if _mldehxg > _qughqyac:
                _qughqyac, _jebpkbt = _mldehxg, 1
            if _merqspwk > _qughqyac:
                _qughqyac, _jebpkbt = _merqspwk, 2
            if _poewph > _qughqyac:
                _qughqyac, _jebpkbt = _poewph, 3
            if _jebpkbt >= 0 and _qughqyac:
                _xmganfndc(_jebpkbt)
                _mjjxtf(_qughqyac)
                _xjilcm(_vrzujg[_mjdabg] if _jebpkbt == 3 else 0)
                _mjdabg += _qughqyac
            else:
                _xmganfndc(4)
                _mjjxtf(_wjboffi + _mjdabg)
                _xjilcm(0)
                _purxhel(_wjboffi + _mjdabg)
                _mjdabg += 1
        _npepfazx.append(len(_cwtwfuzht))
    _vagcuj = np.asarray(_skzeql, np.int32) if _skzeql else np.zeros(0, np.int32)
    _tklikt = _psqbdy.reshape(_zncbwpgl * _wbiurvnv, _tqxphfabb)
    if _vagcuj.size:
        _uwjeui = _tklikt[_vagcuj]
        _amznwh = np.zeros_like(_uwjeui)
        _yjjisi = _vagcuj >= _wbiurvnv
        if np.any(_yjjisi):
            _amznwh[_yjjisi] = _tklikt[_vagcuj[_yjjisi] - _wbiurvnv]
        _fduycpzwr, _mtoruy = _irnebxlo(
            _uwjeui, _amznwh, _yjjisi, _ehvhaps,
            _qisfjgwh if _ehvhaps == 1 else None,
            _qauzlqknx.reshape(_zncbwpgl * _wbiurvnv, _tqxphfabb)[_vagcuj],
        )
        _zegmzrft = np.empty(_zncbwpgl * _wbiurvnv, np.int32)
        _zegmzrft[_vagcuj] = np.arange(_vagcuj.size, dtype=np.int32)
    else:
        _fduycpzwr = np.zeros((0, 1), np.uint8)
        _mtoruy = np.zeros(0, np.uint16)
        _zegmzrft = None

    _mcpvjybtu = bytearray()
    _mcpvjybtu += b"EFC1"
    _mcpvjybtu.append(1)
    _mcpvjybtu.append(_bhbrejner if _ehvhaps == 2 else 0)
    _mcpvjybtu.append(_wnlvmhec)
    _mcpvjybtu.append(0)
    _qhwqirgoe(_mcpvjybtu, _pmyzoizys)
    _qhwqirgoe(_mcpvjybtu, _npwofh)
    _qhwqirgoe(_mcpvjybtu, _lezjrw * _wnlvmhec)
    _qhwqirgoe(_mcpvjybtu, _jfnbkans * _wnlvmhec)
    _qhwqirgoe(_mcpvjybtu, _zncbwpgl)
    _qhwqirgoe(_mcpvjybtu, 0 if _ehvhaps == 2 else len(_qisfjgwh))
    if _ehvhaps == 1:
        for _ctekiux in _qisfjgwh:
            _qhwqirgoe(_mcpvjybtu, int(_ctekiux))
    _lhpbwkwnl = len(_mcpvjybtu)
    _mcpvjybtu.extend(b"\x00" * (_zncbwpgl * 9))
    _gklxpaunh = len(_mcpvjybtu)
    for _rygmly in range(_zncbwpgl):
        _hncubpr = len(_mcpvjybtu)
        if _rygmly == 0 and _eljfqfc >= 0:
            _mcpvjybtu.append(_pvrnyko)
            _mcpvjybtu.append(_eljfqfc)
        for _vjuehi in range(_npepfazx[_rygmly], _npepfazx[_rygmly + 1]):
            _jebpkbt = _cwtwfuzht[_vjuehi]
            _tbklsxopv = _ezdlufu[_vjuehi]
            if _jebpkbt == 0:
                _mcpvjybtu.append(_yjexhmm)
                _eieohxdm(_mcpvjybtu, _tbklsxopv)
            elif _jebpkbt == 1:
                _mcpvjybtu.append(_aolftdpb)
                _eieohxdm(_mcpvjybtu, _tbklsxopv)
            elif _jebpkbt == 2:
                _mcpvjybtu.append(_ldrwcsb)
                _eieohxdm(_mcpvjybtu, _tbklsxopv)
            elif _jebpkbt == 3:
                _mcpvjybtu.append(_cfomrienr)
                _eieohxdm(_mcpvjybtu, _tbklsxopv)
                if _ehvhaps == 1:
                    _mcpvjybtu.append(_mikybpi[_vjuehi] & 255)
                else:
                    _qhwqirgoe(_mcpvjybtu, _mikybpi[_vjuehi])
            else:
                _gvfmqydxt = int(_zegmzrft[_tbklsxopv])
                _mcpvjybtu.extend(_fduycpzwr[_gvfmqydxt, :int(_mtoruy[_gvfmqydxt])])
        _mtscsfe = _hncubpr - _gklxpaunh
        _buecxk = len(_mcpvjybtu) - _hncubpr
        _kuhpqhrh = _lhpbwkwnl + _rygmly * 9
        _mcpvjybtu[_kuhpqhrh] = 1 if _rygmly else 0
        _dmokcvhnc(_mcpvjybtu, _kuhpqhrh + 1, _mtscsfe)
        _dmokcvhnc(_mcpvjybtu, _kuhpqhrh + 5, _buecxk)
    return _mcpvjybtu


def _omxcxffea(blob: bytearray) -> bytes:
    _zncbwpgl = blob[16] | (blob[17] << 8)
    _wcsjfna = blob[18] | (blob[19] << 8)
    _jywtexeh = 20 + _wcsjfna * 2 + _zncbwpgl * 9
    _vfudonk = bytes(blob[_jywtexeh:])
    _evqquzgc = zlib.compressobj(9, zlib.DEFLATED, -15)
    _ggdbbzy = _evqquzgc.compress(_vfudonk) + _evqquzgc.flush()
    if not _ggdbbzy or _jywtexeh + 4 + len(_ggdbbzy) >= len(blob):
        return bytes(blob)
    _mfhfby = bytearray(blob[:_jywtexeh])
    _mfhfby[5] |= _emubcrdl
    _abauvt = len(_vfudonk)
    return bytes(_mfhfby + bytes((_abauvt & 255, (_abauvt >> 8) & 255, (_abauvt >> 16) & 255, (_abauvt >> 24) & 255)) + _ggdbbzy)


def _vjzdyew(_abauvt: int) -> bytes:
    return bytes((_abauvt & 255, (_abauvt >> 8) & 255, (_abauvt >> 16) & 255, (_abauvt >> 24) & 255))


def _qlflyhrr(_vfudonk: bytes) -> bytes:
    if not _vfudonk:
        return _vjzdyew(0) + _vjzdyew(0)
    _evqquzgc = zlib.compressobj(9, zlib.DEFLATED, -15)
    _ggdbbzy = _evqquzgc.compress(_vfudonk) + _evqquzgc.flush()
    if _ggdbbzy and len(_ggdbbzy) < len(_vfudonk):
        return _vjzdyew(len(_vfudonk)) + _vjzdyew(len(_ggdbbzy)) + _ggdbbzy
    return _vjzdyew(len(_vfudonk)) + _vjzdyew(0) + _vfudonk


def _sozuett(_fduycpzwr: bytes, _hmmuonja: bool):
    if not _hmmuonja:
        if len(_fduycpzwr) < 8:
            raise ValueError("truncated predictor planes")
        _zhweod = int.from_bytes(_fduycpzwr[0:4], "little")
        _dcelnlczo = int.from_bytes(_fduycpzwr[4:8], "little")
        _wjboffi = 8
        if len(_fduycpzwr) < _wjboffi + _zhweod + _dcelnlczo:
            raise ValueError("truncated predictor planes")
        _segltpfe = _fduycpzwr[_wjboffi:_wjboffi + _zhweod]
        _hoytyofzp = _fduycpzwr[_wjboffi + _zhweod:_wjboffi + _zhweod + _dcelnlczo]
        return _fduycpzwr[_wjboffi + _zhweod + _dcelnlczo:], _segltpfe, _hoytyofzp

    def _uvwwtvc(_bdapholbj):
        if _bdapholbj + 8 > len(_fduycpzwr):
            raise ValueError("truncated predictor plane")
        _vfudonk = int.from_bytes(_fduycpzwr[_bdapholbj:_bdapholbj + 4], "little")
        _rrgxcbxno = int.from_bytes(_fduycpzwr[_bdapholbj + 4:_bdapholbj + 8], "little")
        _bdapholbj += 8
        if _rrgxcbxno:
            if _bdapholbj + _rrgxcbxno > len(_fduycpzwr):
                raise ValueError("truncated predictor plane")
            _wyddcjqsg = zlib.decompress(_fduycpzwr[_bdapholbj:_bdapholbj + _rrgxcbxno], -15)
            if len(_wyddcjqsg) != _vfudonk:
                raise ValueError("predictor plane length mismatch")
            return _wyddcjqsg, _bdapholbj + _rrgxcbxno
        if _bdapholbj + _vfudonk > len(_fduycpzwr):
            raise ValueError("truncated predictor plane")
        return _fduycpzwr[_bdapholbj:_bdapholbj + _vfudonk], _bdapholbj + _vfudonk

    _knirbjjlp, _bdapholbj = _uvwwtvc(0)
    _segltpfe, _bdapholbj = _uvwwtvc(_bdapholbj)
    _hoytyofzp, _mzwygp = _uvwwtvc(_bdapholbj)
    return _knirbjjlp, _segltpfe, _hoytyofzp


def _igjqhy(_qfotiyyv: np.ndarray, _ukldmm: int, _nwssxun: int) -> np.ndarray:
    _npwofh, _pmyzoizys = _qfotiyyv.shape
    if _npwofh == _ukldmm and _pmyzoizys == _nwssxun:
        return _qfotiyyv
    _ggdbbzy = np.empty((_ukldmm, _nwssxun), _qfotiyyv.dtype)
    _ggdbbzy[:_npwofh, :_pmyzoizys] = _qfotiyyv
    if _pmyzoizys < _nwssxun:
        _ggdbbzy[:_npwofh, _pmyzoizys:] = _qfotiyyv[:, _pmyzoizys - 1:_pmyzoizys]
    if _npwofh < _ukldmm:
        _ggdbbzy[_npwofh:, :] = _ggdbbzy[_npwofh - 1:_npwofh, :]
    return _ggdbbzy


def _mxzhgqp(frames: np.ndarray, _ehvhaps: int, _qisfjgwh: np.ndarray, _mjxenlmkd, _eljfqfc: int,
                 _qkgylznaz: bytearray) -> bytes:

    _zncbwpgl, _npwofh, _pmyzoizys = frames.shape
    _ogfeyczzu = 4
    _lezjrw = (_pmyzoizys + _ogfeyczzu - 1) // _ogfeyczzu
    _jfnbkans = (_npwofh + _ogfeyczzu - 1) // _ogfeyczzu
    _jyybalp = 0 if _ehvhaps == 2 else len(_qisfjgwh)
    _jywtexeh = 20 + _jyybalp * 2 + _zncbwpgl * 9
    _nwujba = 20 + _jyybalp * 2
    _byqmuz = int.from_bytes(_qkgylznaz[_nwujba + 5:_nwujba + 9], "little")
    _knirbjjlp = bytearray(_qkgylznaz[_jywtexeh:_jywtexeh + _byqmuz])
    _zztcqgmy = [0]
    _wyethkko = [_byqmuz]
    _dbgtsgc = _mjxenlmkd[frames] if _ehvhaps == 1 else frames
    _ukldmm, _nwssxun = _jfnbkans * _ogfeyczzu, _lezjrw * _ogfeyczzu
    _segltpfe = bytearray()
    _hoytyofzp = bytearray()
    _tqlfhkgi = _igjqhy(_dbgtsgc[0], _ukldmm, _nwssxun)
    for _rygmly in range(1, _zncbwpgl):
        _zdqppyeb = len(_knirbjjlp)
        _eeqvdxw = _igjqhy(_dbgtsgc[_rygmly], _ukldmm, _nwssxun)
        _acmqoohzd = _eeqvdxw.reshape(_jfnbkans, _ogfeyczzu, _lezjrw, _ogfeyczzu).transpose(0, 2, 1, 3)
        _oaeitxma = _tqlfhkgi.reshape(_jfnbkans, _ogfeyczzu, _lezjrw, _ogfeyczzu).transpose(0, 2, 1, 3)
        _ojyrueypl = np.all(_acmqoohzd == _oaeitxma, axis=(-1, -2))
        _acicln = np.zeros(_eeqvdxw.shape, bool)
        _acicln[:, 1:] = _eeqvdxw[:, 1:] == _eeqvdxw[:, :-1]
        _bjvxmlkof = np.zeros(_eeqvdxw.shape, bool)
        _bjvxmlkof[1:, :] = _eeqvdxw[1:, :] == _eeqvdxw[:-1, :]
        mode = np.full(_eeqvdxw.shape, 3, np.uint8)
        mode[_eeqvdxw == _tqlfhkgi] = 2
        mode[_bjvxmlkof] = 1
        mode[_acicln] = 0
        _uevzim = mode.reshape(_jfnbkans, _ogfeyczzu, _lezjrw, _ogfeyczzu).transpose(0, 2, 1, 3)
        _mjdabg = 0
        _wbiurvnv = _lezjrw * _jfnbkans
        while _mjdabg < _wbiurvnv:
            if _ojyrueypl.reshape(-1)[_mjdabg]:
                _dhjxmqhz = 1
                while _mjdabg + _dhjxmqhz < _wbiurvnv and _ojyrueypl.reshape(-1)[_mjdabg + _dhjxmqhz]:
                    _dhjxmqhz += 1
                _knirbjjlp.append(_yjexhmm)
                _eieohxdm(_knirbjjlp, _dhjxmqhz)
                _mjdabg += _dhjxmqhz
                continue
            _xshoxxfb, _cgdepa = divmod(_mjdabg, _lezjrw)
            _knirbjjlp.append(_ipwvcb)
            _bwqunxw = 0
            _cabeuld = _uevzim[_xshoxxfb, _cgdepa].reshape(-1)
            _qyqszav = _acmqoohzd[_xshoxxfb, _cgdepa].reshape(-1)
            for _bdapholbj in range(16):
                _bwqunxw |= int(_cabeuld[_bdapholbj]) << (2 * _bdapholbj)
                if int(_cabeuld[_bdapholbj]) == 3:
                    _abauvt = int(_qyqszav[_bdapholbj])
                    if _ehvhaps == 1:
                        _hoytyofzp.append(_abauvt & 255)
                    else:
                        _hoytyofzp.append(_abauvt & 255)
                        _hoytyofzp.append((_abauvt >> 8) & 255)
            _segltpfe.extend((_bwqunxw & 255, (_bwqunxw >> 8) & 255, (_bwqunxw >> 16) & 255, (_bwqunxw >> 24) & 255))
            _mjdabg += 1
        _zztcqgmy.append(_zdqppyeb)
        _wyethkko.append(len(_knirbjjlp) - _zdqppyeb)
        _tqlfhkgi = _eeqvdxw
    _ixigohfh = bytearray(_qkgylznaz[:_jywtexeh])
    _ixigohfh[5] |= _qgbcnn | _emubcrdl
    for _rygmly in range(_zncbwpgl):
        _fgjujq = _nwujba + _rygmly * 9
        _ixigohfh[_fgjujq] = 0 if _rygmly == 0 else 1
        _dmokcvhnc(_ixigohfh, _fgjujq + 1, _zztcqgmy[_rygmly])
        _dmokcvhnc(_ixigohfh, _fgjujq + 5, _wyethkko[_rygmly])
    return bytes(_ixigohfh) + _qlflyhrr(bytes(_knirbjjlp)) + _qlflyhrr(bytes(_segltpfe)) + _qlflyhrr(bytes(_hoytyofzp))


def dither_hold(frames: np.ndarray, color_key: int | None = None) -> np.ndarray:

    _uuaatbvx = np.ascontiguousarray(frames, dtype="<u2")
    _zncbwpgl, _npwofh, _pmyzoizys = _uuaatbvx.shape
    _jyghohi = _uuaatbvx.copy()
    if _zncbwpgl <= 1:
        return _jyghohi

    def _xbhejt(_axytzuca: np.ndarray) -> np.ndarray:
        _znanqy = np.pad(_axytzuca, 1, mode="edge")
        _abauvt = np.stack([_znanqy[_tembga:_tembga + _npwofh, _dtptfhpn:_dtptfhpn + _pmyzoizys] for _tembga in range(3) for _dtptfhpn in range(3)], 0)
        _abauvt.sort(axis=0)
        return _abauvt[4]

    _zjxjnig = _xbhejt(_uuaatbvx[0])
    for _bdapholbj in range(1, _zncbwpgl):
        _crybazehs = _xbhejt(_uuaatbvx[_bdapholbj])
        _ojyrueypl = _crybazehs == _zjxjnig
        if color_key is not None:
            _uzmcstl = np.uint16(color_key)
            _ojyrueypl &= _uuaatbvx[_bdapholbj] != _uzmcstl
            _ojyrueypl &= _jyghohi[_bdapholbj - 1] != _uzmcstl
        _jyghohi[_bdapholbj][_ojyrueypl] = _jyghohi[_bdapholbj - 1][_ojyrueypl]
        _zjxjnig = _crybazehs
    return _jyghohi


def encode_frames(frames: np.ndarray, tile: int = 4, color_key: int | None = None,
                  compress: bool = True, legacy: bool = False, hold: bool = False) -> bytes:

    frames = np.ascontiguousarray(frames, dtype="<u2")
    if frames.ndim != 3:
        raise ValueError("frames must be (count, height, width)")
    if hold:
        frames = dither_hold(frames, color_key)
    _zncbwpgl, _npwofh, _pmyzoizys = (int(frames.shape[0]), int(frames.shape[1]), int(frames.shape[2]))
    if _zncbwpgl <= 0 or _npwofh <= 0 or _pmyzoizys <= 0:
        raise ValueError("bad geometry")
    _qisfjgwh = np.flatnonzero(np.bincount(frames.reshape(-1), minlength=65536)).astype("<u2")
    if _qisfjgwh.size <= 256:
        _ehvhaps = 1
        _mjxenlmkd = np.full(65536, -1, np.int16)
        _mjxenlmkd[_qisfjgwh] = np.arange(_qisfjgwh.size, dtype=np.int16)
    else:
        _ehvhaps = 2
        _qisfjgwh = np.empty(0, np.uint16)
        _mjxenlmkd = None
    _eljfqfc = -1
    if _ehvhaps == 1 and color_key is not None and 0 <= int(color_key) <= 65535 and int(_mjxenlmkd[int(color_key)]) >= 0:
        _eljfqfc = int(_mjxenlmkd[int(color_key)])
    _segltpfe = (4, 8) if tile == 0 else (tile,)
    _qughqyac = None
    _cemsbmaj = None
    for _wnlvmhec in _segltpfe:
        if _wnlvmhec not in (4, 8):
            raise ValueError("tile must be 4, 8, or 0")
        blob = _uhtzzygz(frames, _wnlvmhec, _ehvhaps, _qisfjgwh, _mjxenlmkd, _eljfqfc)
        _vfudonk = bytes(blob)
        if _qughqyac is None or len(_vfudonk) < len(_qughqyac):
            _qughqyac = _vfudonk
        if _wnlvmhec == 4:
            _cemsbmaj = blob
    if compress:
        _qughqyac = _omxcxffea(bytearray(_qughqyac))
        if 4 in _segltpfe and _cemsbmaj is not None and not legacy:
            _kouwhqg = _mxzhgqp(frames, _ehvhaps, _qisfjgwh, _mjxenlmkd, _eljfqfc, _cemsbmaj)
            if len(_kouwhqg) < len(_qughqyac):
                _qughqyac = _kouwhqg
    if _ehvhaps == 2 and color_key is not None:
        _cvmqvj = bytearray(_qughqyac)
        _cvmqvj[7] = 1
        _qughqyac = bytes(_cvmqvj)
    return _qughqyac


def _ktqtqgwio(_aburltus, _bdapholbj: int, _ehvhaps: int, _wcsjfna) -> tuple[int, int]:
    if _ehvhaps == 1:
        return int(_wcsjfna[_aburltus[_bdapholbj]]), _bdapholbj + 1
    return _aburltus[_bdapholbj] | (_aburltus[_bdapholbj + 1] << 8), _bdapholbj + 2


def _qereqqaoy(_tklikt, _qwaanxhsj, _tseapbpi, _pajocq, _qphodvu, _vprmqjwgh):
    _ctekiux = (_tseapbpi, _pajocq)
    _tklikt[_qwaanxhsj] = _ctekiux[_qphodvu & 1]
    _tklikt[_qwaanxhsj + 1] = _ctekiux[(_qphodvu >> 1) & 1]
    _tklikt[_qwaanxhsj + 2] = _ctekiux[(_qphodvu >> 2) & 1]
    _tklikt[_qwaanxhsj + 3] = _ctekiux[(_qphodvu >> 3) & 1]
    _tklikt[_qwaanxhsj + 4] = _ctekiux[(_qphodvu >> 4) & 1]
    _tklikt[_qwaanxhsj + 5] = _ctekiux[(_qphodvu >> 5) & 1]
    _tklikt[_qwaanxhsj + 6] = _ctekiux[(_qphodvu >> 6) & 1]
    _tklikt[_qwaanxhsj + 7] = _ctekiux[(_qphodvu >> 7) & 1]
    _tklikt[_qwaanxhsj + 8] = _ctekiux[_vprmqjwgh & 1]
    _tklikt[_qwaanxhsj + 9] = _ctekiux[(_vprmqjwgh >> 1) & 1]
    _tklikt[_qwaanxhsj + 10] = _ctekiux[(_vprmqjwgh >> 2) & 1]
    _tklikt[_qwaanxhsj + 11] = _ctekiux[(_vprmqjwgh >> 3) & 1]
    _tklikt[_qwaanxhsj + 12] = _ctekiux[(_vprmqjwgh >> 4) & 1]
    _tklikt[_qwaanxhsj + 13] = _ctekiux[(_vprmqjwgh >> 5) & 1]
    _tklikt[_qwaanxhsj + 14] = _ctekiux[(_vprmqjwgh >> 6) & 1]
    _tklikt[_qwaanxhsj + 15] = _ctekiux[(_vprmqjwgh >> 7) & 1]


def _tayzefe(_tklikt, _qwaanxhsj, _ctekiux, _qphodvu, _vprmqjwgh, _gwxuemlb, _acuxinf):
    _tklikt[_qwaanxhsj] = _ctekiux[_qphodvu & 3]
    _tklikt[_qwaanxhsj + 1] = _ctekiux[(_qphodvu >> 2) & 3]
    _tklikt[_qwaanxhsj + 2] = _ctekiux[(_qphodvu >> 4) & 3]
    _tklikt[_qwaanxhsj + 3] = _ctekiux[(_qphodvu >> 6) & 3]
    _tklikt[_qwaanxhsj + 4] = _ctekiux[_vprmqjwgh & 3]
    _tklikt[_qwaanxhsj + 5] = _ctekiux[(_vprmqjwgh >> 2) & 3]
    _tklikt[_qwaanxhsj + 6] = _ctekiux[(_vprmqjwgh >> 4) & 3]
    _tklikt[_qwaanxhsj + 7] = _ctekiux[(_vprmqjwgh >> 6) & 3]
    _tklikt[_qwaanxhsj + 8] = _ctekiux[_gwxuemlb & 3]
    _tklikt[_qwaanxhsj + 9] = _ctekiux[(_gwxuemlb >> 2) & 3]
    _tklikt[_qwaanxhsj + 10] = _ctekiux[(_gwxuemlb >> 4) & 3]
    _tklikt[_qwaanxhsj + 11] = _ctekiux[(_gwxuemlb >> 6) & 3]
    _tklikt[_qwaanxhsj + 12] = _ctekiux[_acuxinf & 3]
    _tklikt[_qwaanxhsj + 13] = _ctekiux[(_acuxinf >> 2) & 3]
    _tklikt[_qwaanxhsj + 14] = _ctekiux[(_acuxinf >> 4) & 3]
    _tklikt[_qwaanxhsj + 15] = _ctekiux[(_acuxinf >> 6) & 3]


def _txdrbkvqq(_tklikt, _qwaanxhsj, _ctekiux, _iauczi):
    _tklikt[_qwaanxhsj] = _ctekiux[_iauczi & 7]
    _tklikt[_qwaanxhsj + 1] = _ctekiux[(_iauczi >> 3) & 7]
    _tklikt[_qwaanxhsj + 2] = _ctekiux[(_iauczi >> 6) & 7]
    _tklikt[_qwaanxhsj + 3] = _ctekiux[(_iauczi >> 9) & 7]
    _tklikt[_qwaanxhsj + 4] = _ctekiux[(_iauczi >> 12) & 7]
    _tklikt[_qwaanxhsj + 5] = _ctekiux[(_iauczi >> 15) & 7]
    _tklikt[_qwaanxhsj + 6] = _ctekiux[(_iauczi >> 18) & 7]
    _tklikt[_qwaanxhsj + 7] = _ctekiux[(_iauczi >> 21) & 7]
    _tklikt[_qwaanxhsj + 8] = _ctekiux[(_iauczi >> 24) & 7]
    _tklikt[_qwaanxhsj + 9] = _ctekiux[(_iauczi >> 27) & 7]
    _tklikt[_qwaanxhsj + 10] = _ctekiux[(_iauczi >> 30) & 7]
    _tklikt[_qwaanxhsj + 11] = _ctekiux[(_iauczi >> 33) & 7]
    _tklikt[_qwaanxhsj + 12] = _ctekiux[(_iauczi >> 36) & 7]
    _tklikt[_qwaanxhsj + 13] = _ctekiux[(_iauczi >> 39) & 7]
    _tklikt[_qwaanxhsj + 14] = _ctekiux[(_iauczi >> 42) & 7]
    _tklikt[_qwaanxhsj + 15] = _ctekiux[(_iauczi >> 45) & 7]


def _wfqagqlew(_wnuekdv: bytes, _gazjxqcgp, _wcsjfna: np.ndarray, _ehvhaps: int, _pmyzoizys: int, _npwofh: int,
               _segltpfe: bytes | None = None, _hoytyofzp: bytes | None = None):

    _wnlvmhec = 4
    _lezjrw = (_pmyzoizys + _wnlvmhec - 1) // _wnlvmhec
    _jfnbkans = (_npwofh + _wnlvmhec - 1) // _wnlvmhec
    _wbiurvnv = _lezjrw * _jfnbkans
    _zncbwpgl = len(_gazjxqcgp)
    _ggdbbzy = np.empty((_zncbwpgl, _npwofh, _pmyzoizys), np.uint16)
    _eeqvdxw = np.zeros((_wbiurvnv, 16), np.uint16)
    _tklikt = memoryview(_eeqvdxw).cast("B").cast("H")
    _aburltus = memoryview(_wnuekdv)
    _pwzfps = tuple(int(_abauvt) for _abauvt in _wcsjfna) if _ehvhaps == 1 else ()
    color_key = None
    _addtfl = _ehvhaps == 1
    _abaslj = memoryview(_segltpfe) if _segltpfe is not None else None
    _unjshnmv = memoryview(_hoytyofzp) if _hoytyofzp is not None else None
    _qlrhns = 0
    _oqofaatp = 0
    for _rygmly, (_mtscsfe, _buecxk) in enumerate(_gazjxqcgp):
        _znanqy = _mtscsfe
        _xdcfkgfu = _mtscsfe + _buecxk
        if _znanqy < _xdcfkgfu and _aburltus[_znanqy] == _pvrnyko:
            if not _addtfl or _znanqy + 1 >= _xdcfkgfu or _aburltus[_znanqy + 1] >= len(_pwzfps):
                raise ValueError("bad color-key opcode")
            color_key = _pwzfps[_aburltus[_znanqy + 1]]
            _znanqy += 2
        _mjdabg = 0
        while _znanqy < _xdcfkgfu and _mjdabg < _wbiurvnv:
            _waabewu = _aburltus[_znanqy]
            _znanqy += 1
            if _waabewu == _cfomrienr:
                _dhjxmqhz, _znanqy = _fvhfwnqmk(_aburltus, _znanqy)
                _ctekiux, _znanqy = _ktqtqgwio(_aburltus, _znanqy, _ehvhaps, _pwzfps if _addtfl else _wcsjfna)
                _eeqvdxw[_mjdabg:_mjdabg + _dhjxmqhz] = _ctekiux
                _mjdabg += _dhjxmqhz
            elif _waabewu == _yjexhmm:
                _dhjxmqhz, _znanqy = _fvhfwnqmk(_aburltus, _znanqy)
                _mjdabg += _dhjxmqhz
            elif _waabewu == _aolftdpb:
                _dhjxmqhz, _znanqy = _fvhfwnqmk(_aburltus, _znanqy)
                if _mjdabg > 0 and _dhjxmqhz:
                    _eeqvdxw[_mjdabg:_mjdabg + _dhjxmqhz] = _eeqvdxw[_mjdabg - 1]
                _mjdabg += _dhjxmqhz
            elif _waabewu == _ldrwcsb:
                _dhjxmqhz, _znanqy = _fvhfwnqmk(_aburltus, _znanqy)
                if _mjdabg >= _lezjrw and _dhjxmqhz:
                    if _dhjxmqhz <= _lezjrw:
                        _eeqvdxw[_mjdabg:_mjdabg + _dhjxmqhz] = _eeqvdxw[_mjdabg - _lezjrw:_mjdabg - _lezjrw + _dhjxmqhz]
                    else:
                        _uuaatbvx = _eeqvdxw[_mjdabg - _lezjrw:_mjdabg]
                        _ukyampd = (_dhjxmqhz + _lezjrw - 1) // _lezjrw
                        _eeqvdxw[_mjdabg:_mjdabg + _dhjxmqhz] = np.tile(_uuaatbvx, (_ukyampd, 1))[:_dhjxmqhz]
                _mjdabg += _dhjxmqhz
            elif _waabewu == _usvojwmv:
                _qwaanxhsj = _mjdabg * 16
                if _addtfl:
                    for _bdapholbj in range(16):
                        _tklikt[_qwaanxhsj + _bdapholbj] = _pwzfps[_aburltus[_znanqy]]
                        _znanqy += 1
                else:
                    for _bdapholbj in range(16):
                        _tklikt[_qwaanxhsj + _bdapholbj] = _aburltus[_znanqy] | (_aburltus[_znanqy + 1] << 8)
                        _znanqy += 2
                _mjdabg += 1
            elif _waabewu == _ljhzfwyhc:
                if _addtfl:
                    _glkcxlakr = _pwzfps[_aburltus[_znanqy]]
                    _ibaaxq = _pwzfps[_aburltus[_znanqy + 1]]
                    _znanqy += 2
                else:
                    _glkcxlakr = _aburltus[_znanqy] | (_aburltus[_znanqy + 1] << 8)
                    _znanqy += 2
                    _ibaaxq = _aburltus[_znanqy] | (_aburltus[_znanqy + 1] << 8)
                    _znanqy += 2
                _qereqqaoy(_tklikt, _mjdabg * 16, _glkcxlakr, _ibaaxq, _aburltus[_znanqy], _aburltus[_znanqy + 1])
                _znanqy += 2
                _mjdabg += 1
            elif _waabewu == _qpkxojtc:
                if _addtfl:
                    _tseapbpi, _pajocq, _hppmafap, _akykmdgw = _pwzfps[_aburltus[_znanqy]], _pwzfps[_aburltus[_znanqy + 1]], _pwzfps[_aburltus[_znanqy + 2]], _pwzfps[_aburltus[_znanqy + 3]]
                    _znanqy += 4
                else:
                    _tseapbpi = _aburltus[_znanqy] | (_aburltus[_znanqy + 1] << 8)
                    _pajocq = _aburltus[_znanqy + 2] | (_aburltus[_znanqy + 3] << 8)
                    _hppmafap = _aburltus[_znanqy + 4] | (_aburltus[_znanqy + 5] << 8)
                    _akykmdgw = _aburltus[_znanqy + 6] | (_aburltus[_znanqy + 7] << 8)
                    _znanqy += 8
                _tayzefe(_tklikt, _mjdabg * 16, (_tseapbpi, _pajocq, _hppmafap, _akykmdgw), _aburltus[_znanqy], _aburltus[_znanqy + 1], _aburltus[_znanqy + 2], _aburltus[_znanqy + 3])
                _znanqy += 4
                _mjdabg += 1
            elif _waabewu == _qnfyke:
                if _addtfl:
                    _mikybpi = tuple(_pwzfps[_aburltus[_znanqy + _bdapholbj]] for _bdapholbj in range(8))
                    _znanqy += 8
                else:
                    _mikybpi = tuple(_aburltus[_znanqy + 2 * _bdapholbj] | (_aburltus[_znanqy + 2 * _bdapholbj + 1] << 8) for _bdapholbj in range(8))
                    _znanqy += 16
                _iauczi = int.from_bytes(_aburltus[_znanqy:_znanqy + 6], "little")
                _txdrbkvqq(_tklikt, _mjdabg * 16, _mikybpi, _iauczi)
                _znanqy += 6
                _mjdabg += 1
            elif _waabewu == _wltpszoc:
                _sloyfeabr = _aburltus[_znanqy]
                _znanqy += 1
                _qwaanxhsj = _mjdabg * 16
                for _nzukczx in range(_sloyfeabr):
                    _nrqfmuho = _aburltus[_znanqy]
                    _znanqy += 1
                    _ctekiux, _znanqy = _ktqtqgwio(_aburltus, _znanqy, _ehvhaps, _pwzfps if _addtfl else _wcsjfna)
                    _tklikt[_qwaanxhsj + _nrqfmuho] = _ctekiux
                _mjdabg += 1
            elif _waabewu == _nbvcyrszk:
                _oeiqpgsi = _aburltus[_znanqy] | (_aburltus[_znanqy + 1] << 8)
                _znanqy += 2
                _qwaanxhsj = _mjdabg * 16
                _ykxpejxtu = 1
                for _bdapholbj in range(16):
                    if _oeiqpgsi & _ykxpejxtu:
                        _ctekiux, _znanqy = _ktqtqgwio(_aburltus, _znanqy, _ehvhaps, _pwzfps if _addtfl else _wcsjfna)
                        _tklikt[_qwaanxhsj + _bdapholbj] = _ctekiux
                    _ykxpejxtu <<= 1
                _mjdabg += 1
            elif _waabewu == _ipwvcb:
                if _abaslj is None or _unjshnmv is None or _qlrhns + 4 > len(_abaslj):
                    raise ValueError("truncated predictor modes")
                _bwqunxw = (_abaslj[_qlrhns] | (_abaslj[_qlrhns + 1] << 8) |
                        (_abaslj[_qlrhns + 2] << 16) | (_abaslj[_qlrhns + 3] << 24))
                _qlrhns += 4
                _xshoxxfb, _cgdepa = divmod(_mjdabg, _lezjrw)
                tile = _eeqvdxw[_mjdabg]
                for _bdapholbj in range(16):
                    mode = (_bwqunxw >> (2 * _bdapholbj)) & 3
                    if mode == 2:
                        continue
                    if mode == 0:
                        if _bdapholbj & 3:
                            tile[_bdapholbj] = tile[_bdapholbj - 1]
                        elif _cgdepa:
                            tile[_bdapholbj] = _eeqvdxw[_mjdabg - 1][_bdapholbj + 3]
                        else:
                            raise ValueError("predictor left edge")
                    elif mode == 1:
                        if _bdapholbj >= 4:
                            tile[_bdapholbj] = tile[_bdapholbj - 4]
                        elif _xshoxxfb:
                            tile[_bdapholbj] = _eeqvdxw[_mjdabg - _lezjrw][_bdapholbj + 12]
                        else:
                            raise ValueError("predictor top edge")
                    else:
                        if _addtfl:
                            if _oqofaatp >= len(_unjshnmv) or _unjshnmv[_oqofaatp] >= len(_pwzfps):
                                raise ValueError("truncated predictor literal")
                            tile[_bdapholbj] = _pwzfps[_unjshnmv[_oqofaatp]]
                            _oqofaatp += 1
                        else:
                            if _oqofaatp + 2 > len(_unjshnmv):
                                raise ValueError("truncated predictor literal")
                            tile[_bdapholbj] = _unjshnmv[_oqofaatp] | (_unjshnmv[_oqofaatp + 1] << 8)
                            _oqofaatp += 2
                _mjdabg += 1
            else:
                raise ValueError(f"unknown opcode {op:#x} in frame {f}")
        if _mjdabg != _wbiurvnv:
            raise ValueError(f"frame {f} covered {t} tiles, expected {ntiles}")
        _qfotiyyv = _eeqvdxw.reshape(_jfnbkans, _lezjrw, _wnlvmhec, _wnlvmhec).transpose(0, 2, 1, 3).reshape(_jfnbkans * _wnlvmhec, _lezjrw * _wnlvmhec)
        _ggdbbzy[_rygmly] = _qfotiyyv[:_npwofh, :_pmyzoizys]
    return _ggdbbzy, color_key


def _pjcedxkxi(_wnuekdv: bytes, _gazjxqcgp, _wcsjfna: np.ndarray, _ehvhaps: int, _pmyzoizys: int, _npwofh: int, _wnlvmhec: int):
    _lezjrw = (_pmyzoizys + _wnlvmhec - 1) // _wnlvmhec
    _jfnbkans = (_npwofh + _wnlvmhec - 1) // _wnlvmhec
    _txaetjsn, _hqqhuzdu = _lezjrw * _wnlvmhec, _jfnbkans * _wnlvmhec
    _wbiurvnv = _lezjrw * _jfnbkans
    _tqxphfabb = _wnlvmhec * _wnlvmhec
    _zncbwpgl = len(_gazjxqcgp)
    _ggdbbzy = np.empty((_zncbwpgl, _npwofh, _pmyzoizys), np.uint16)
    _eeqvdxw = np.zeros((_wbiurvnv, _tqxphfabb), np.uint16)
    _aburltus = memoryview(_wnuekdv)
    color_key = None
    for _rygmly, (_mtscsfe, _buecxk) in enumerate(_gazjxqcgp):
        _znanqy = _mtscsfe
        _xdcfkgfu = _mtscsfe + _buecxk
        if _znanqy < _xdcfkgfu and _aburltus[_znanqy] == _pvrnyko:
            if _ehvhaps != 1 or _znanqy + 1 >= _xdcfkgfu or _aburltus[_znanqy + 1] >= len(_wcsjfna):
                raise ValueError("bad color-key opcode")
            color_key = int(_wcsjfna[_aburltus[_znanqy + 1]])
            _znanqy += 2
        _mjdabg = 0
        while _znanqy < _xdcfkgfu and _mjdabg < _wbiurvnv:
            _waabewu = _aburltus[_znanqy]
            _znanqy += 1
            if _waabewu == _cfomrienr:
                _dhjxmqhz, _znanqy = _fvhfwnqmk(_aburltus, _znanqy)
                _ctekiux, _znanqy = _ktqtqgwio(_aburltus, _znanqy, _ehvhaps, _wcsjfna)
                _eeqvdxw[_mjdabg:_mjdabg + _dhjxmqhz] = _ctekiux
                _mjdabg += _dhjxmqhz
            elif _waabewu == _yjexhmm:
                _dhjxmqhz, _znanqy = _fvhfwnqmk(_aburltus, _znanqy)
                _mjdabg += _dhjxmqhz
            elif _waabewu == _aolftdpb:
                _dhjxmqhz, _znanqy = _fvhfwnqmk(_aburltus, _znanqy)
                for _tbdaserd in range(_dhjxmqhz):
                    if _mjdabg > 0:
                        _eeqvdxw[_mjdabg] = _eeqvdxw[_mjdabg - 1]
                    _mjdabg += 1
            elif _waabewu == _ldrwcsb:
                _dhjxmqhz, _znanqy = _fvhfwnqmk(_aburltus, _znanqy)
                for _tbdaserd in range(_dhjxmqhz):
                    if _mjdabg >= _lezjrw:
                        _eeqvdxw[_mjdabg] = _eeqvdxw[_mjdabg - _lezjrw]
                    _mjdabg += 1
            elif _waabewu == _usvojwmv:
                _gvfmqydxt = _eeqvdxw[_mjdabg]
                for _bdapholbj in range(_tqxphfabb):
                    _gvfmqydxt[_bdapholbj], _znanqy = _ktqtqgwio(_aburltus, _znanqy, _ehvhaps, _wcsjfna)
                _mjdabg += 1
            elif _waabewu in (_ljhzfwyhc, _qpkxojtc, _qnfyke):
                _xxpizc = 1 if _waabewu == _ljhzfwyhc else 2 if _waabewu == _qpkxojtc else 3
                _nnayzlug = 1 << _xxpizc
                _pjzzfvz = [0] * _nnayzlug
                for _njvjnbjx in range(_nnayzlug):
                    _pjzzfvz[_njvjnbjx], _znanqy = _ktqtqgwio(_aburltus, _znanqy, _ehvhaps, _wcsjfna)
                _qijusvkx = (_tqxphfabb * _xxpizc + 7) >> 3
                _klbgfmf = _aburltus[_znanqy:_znanqy + _qijusvkx]
                _znanqy += _qijusvkx
                _gvfmqydxt = _eeqvdxw[_mjdabg]
                for _bdapholbj in range(_tqxphfabb):
                    _uhjprvmdf = 0
                    _ykxpejxtu = _bdapholbj * _xxpizc
                    for _ibaaxq in range(_xxpizc):
                        _iaozcxqrc = _ykxpejxtu + _ibaaxq
                        if _klbgfmf[_iaozcxqrc >> 3] & (1 << (_iaozcxqrc & 7)):
                            _uhjprvmdf |= 1 << _ibaaxq
                    _gvfmqydxt[_bdapholbj] = _pjzzfvz[_uhjprvmdf]
                _mjdabg += 1
            elif _waabewu == _wltpszoc:
                _sloyfeabr = _aburltus[_znanqy]
                _znanqy += 1
                _gvfmqydxt = _eeqvdxw[_mjdabg]
                for _tbdaserd in range(_sloyfeabr):
                    _nrqfmuho = _aburltus[_znanqy]
                    _znanqy += 1
                    _gvfmqydxt[_nrqfmuho], _znanqy = _ktqtqgwio(_aburltus, _znanqy, _ehvhaps, _wcsjfna)
                _mjdabg += 1
            elif _waabewu == _nbvcyrszk:
                _qijusvkx = (_tqxphfabb + 7) >> 3
                _klbgfmf = _aburltus[_znanqy:_znanqy + _qijusvkx]
                _znanqy += _qijusvkx
                _gvfmqydxt = _eeqvdxw[_mjdabg]
                for _bdapholbj in range(_tqxphfabb):
                    if _klbgfmf[_bdapholbj >> 3] & (1 << (_bdapholbj & 7)):
                        _gvfmqydxt[_bdapholbj], _znanqy = _ktqtqgwio(_aburltus, _znanqy, _ehvhaps, _wcsjfna)
                _mjdabg += 1
            else:
                raise ValueError(f"unknown opcode {op:#x} in frame {f}")
        if _mjdabg != _wbiurvnv:
            raise ValueError(f"frame {f} covered {t} tiles, expected {ntiles}")
        _qfotiyyv = _eeqvdxw.reshape(_jfnbkans, _lezjrw, _wnlvmhec, _wnlvmhec).transpose(0, 2, 1, 3).reshape(_hqqhuzdu, _txaetjsn)
        _ggdbbzy[_rygmly] = _qfotiyyv[:_npwofh, :_pmyzoizys]
    return _ggdbbzy, color_key


def _yqjucw(_wnuekdv: bytes, _gazjxqcgp, _wcsjfna: np.ndarray, _ehvhaps: int, _pmyzoizys: int, _npwofh: int, _wnlvmhec: int,
                   _segltpfe: bytes | None = None, _hoytyofzp: bytes | None = None):
    if _wnlvmhec == 4:
        return _wfqagqlew(_wnuekdv, _gazjxqcgp, _wcsjfna, _ehvhaps, _pmyzoizys, _npwofh, _segltpfe, _hoytyofzp)
    return _pjcedxkxi(_wnuekdv, _gazjxqcgp, _wcsjfna, _ehvhaps, _pmyzoizys, _npwofh, _wnlvmhec)


def _zydrbhgib(frames: np.ndarray, _nialijbw: int, _yuujpd: int,
              color_key: int | None, _kytqpjtx: int = 0) -> np.ndarray:

    _xaazipig, _dutwtl, _txugak = frames.shape

    def _joqvfil(_dzyugjk: int, _blkebgi: int):
        if _kytqpjtx:
            _nrqfmuho = np.arange(_dzyugjk, dtype=np.int32)
            _actxwdg = _nrqfmuho % _kytqpjtx
            _krnite = np.array(
                [(_bdapholbj * (_kytqpjtx - 1) + 1) // 3 for _bdapholbj in range(4)],
                dtype=np.int32,
            )
            _ujkppnt = np.sum(_actxwdg[:, None] >= _krnite[None, 1:], axis=1)
            _wjboffi = (_nrqfmuho // _kytqpjtx) * 4 + _ujkppnt
            _grvcywmd = np.minimum(_wjboffi + 1, _blkebgi - 1)
            _jibtuwzt = np.zeros(_dzyugjk, np.uint32)
            _thcido = _ujkppnt < 3
            _owamkqtjq = _krnite[_ujkppnt[_thcido]]
            _dzhdci = _krnite[_ujkppnt[_thcido] + 1]
            _jibtuwzt[_thcido] = (
                (_actxwdg[_thcido] - _owamkqtjq) * 256 // (_dzhdci - _owamkqtjq)
            ).astype(np.uint32)
            _wjboffi = np.minimum(_wjboffi, _blkebgi - 1)
            return _wjboffi, _grvcywmd, _jibtuwzt
        if _dzyugjk <= 1:
            _ixwwki = np.zeros(_dzyugjk, np.uint32)
        else:
            _ixwwki = ((np.arange(_dzyugjk, dtype=np.uint64) * (_blkebgi - 1) * 256
                   + (_dzyugjk - 1) // 2) // (_dzyugjk - 1)).astype(np.uint32)
        _wjboffi = (_ixwwki >> 8).astype(np.int32)
        _jibtuwzt = (_ixwwki & 255).astype(np.uint32)
        _nbwbfmo = _wjboffi >= _blkebgi - 1
        _wjboffi[_nbwbfmo] = _blkebgi - 1
        _jibtuwzt[_nbwbfmo] = 0
        _grvcywmd = np.minimum(_wjboffi + 1, _blkebgi - 1)
        return _wjboffi, _grvcywmd, _jibtuwzt

    _isyazmo, _rzlych, _mmvmtyh = _joqvfil(_nialijbw, _txugak)
    _sqiqkpqsg, _ggcfzwj, _jezsei = _joqvfil(_yuujpd, _dutwtl)

    def _nqlbephtu(_glkcxlakr, _ibaaxq, _jibtuwzt):
        _glkcxlakr = _glkcxlakr.astype(np.uint32)
        _ibaaxq = _ibaaxq.astype(np.uint32)
        _urcufmw = 256 - _jibtuwzt
        _vxuyuws = (((_glkcxlakr >> 11) * _urcufmw + (_ibaaxq >> 11) * _jibtuwzt + 128) >> 8)
        _fndcitcg = ((((_glkcxlakr >> 5) & 63) * _urcufmw + ((_ibaaxq >> 5) & 63) * _jibtuwzt + 128) >> 8)
        _lfakmcp = (((_glkcxlakr & 31) * _urcufmw + (_ibaaxq & 31) * _jibtuwzt + 128) >> 8)
        return ((_vxuyuws << 11) | (_fndcitcg << 5) | _lfakmcp).astype(np.uint16)

    _ggdbbzy = np.empty((len(frames), _yuujpd, _nialijbw), np.uint16)
    _jwakiinfs = _mmvmtyh[None, :]
    _lrcrrpzmg = _jezsei[:, None]
    if _kytqpjtx:
        def _mpyzzz(_glkcxlakr, _ibaaxq, _jibtuwzt):
            if color_key is None:
                return _nqlbephtu(_glkcxlakr, _ibaaxq, _jibtuwzt)
            _uzmcstl = np.uint16(color_key)
            _wramnxocx = np.where(_jibtuwzt >= 128, _ibaaxq, _glkcxlakr)
            _pynrdcu = _wramnxocx == _uzmcstl
            _byfiyenv = np.where(_glkcxlakr == _uzmcstl, _wramnxocx, _glkcxlakr)
            _ailkrkhbj = np.where(_ibaaxq == _uzmcstl, _wramnxocx, _ibaaxq)
            _irgfwz = _nqlbephtu(_byfiyenv, _ailkrkhbj, _jibtuwzt)
            _irgfwz = np.where(_pynrdcu, _uzmcstl, _irgfwz)
            return np.where(
                ~_pynrdcu & (_irgfwz == _uzmcstl), _irgfwz ^ np.uint16(1), _irgfwz
            ).astype(np.uint16)

        for _rygmly, _uuaatbvx in enumerate(frames):
            _ejsgkcm = _mpyzzz(
                _uuaatbvx[:, _isyazmo], _uuaatbvx[:, _rzlych], _jwakiinfs
            )
            _ggdbbzy[_rygmly] = _mpyzzz(
                _ejsgkcm[_sqiqkpqsg], _ejsgkcm[_ggcfzwj], _lrcrrpzmg
            )
        return _ggdbbzy

    _jugcqafi = np.where(_mmvmtyh >= 128, _rzlych, _isyazmo)
    _thmuwvrp = np.where(_jezsei >= 128, _ggcfzwj, _sqiqkpqsg)
    for _rygmly, _uuaatbvx in enumerate(frames):
        _wramnxocx = _uuaatbvx[_thmuwvrp[:, None], _jugcqafi[None, :]]
        _wgvpjw = _uuaatbvx[_sqiqkpqsg[:, None], _isyazmo[None, :]]
        _oooydrgj = _uuaatbvx[_sqiqkpqsg[:, None], _rzlych[None, :]]
        _imggcscex = _uuaatbvx[_ggcfzwj[:, None], _isyazmo[None, :]]
        _letuqidw = _uuaatbvx[_ggcfzwj[:, None], _rzlych[None, :]]
        if color_key is not None:
            _uzmcstl = np.uint16(color_key)
            _wgvpjw = np.where(_wgvpjw == _uzmcstl, _wramnxocx, _wgvpjw)
            _oooydrgj = np.where(_oooydrgj == _uzmcstl, _wramnxocx, _oooydrgj)
            _imggcscex = np.where(_imggcscex == _uzmcstl, _wramnxocx, _imggcscex)
            _letuqidw = np.where(_letuqidw == _uzmcstl, _wramnxocx, _letuqidw)
        _swmocavmt = _nqlbephtu(_wgvpjw, _oooydrgj, _jwakiinfs)
        _orbuaurvh = _nqlbephtu(_imggcscex, _letuqidw, _jwakiinfs)
        _irgfwz = _nqlbephtu(_swmocavmt, _orbuaurvh, _lrcrrpzmg)
        if color_key is not None:
            _pynrdcu = _wramnxocx == _uzmcstl
            _irgfwz = np.where(_pynrdcu, _uzmcstl, _irgfwz)
            _irgfwz = np.where(~_pynrdcu & (_irgfwz == _uzmcstl), _irgfwz ^ np.uint16(1), _irgfwz)
        _ggdbbzy[_rygmly] = _irgfwz
    return _ggdbbzy


def decode_frames(blob: bytes) -> tuple[np.ndarray, dict]:

    if len(blob) < 20 or blob[:4] != b"EFC1":
        raise ValueError("not an EFC1 file")
    _bmboczbig = blob[5]
    if _bmboczbig & _hcrnbs:
        raise ValueError("quadtree EFC is not supported by the Python codec")
    _wnlvmhec = blob[6]
    if _wnlvmhec not in (4, 8):
        raise ValueError(f"unsupported tile size {S}")
    _pmyzoizys, _npwofh, _txaetjsn, _hqqhuzdu, _zncbwpgl, _jyybalp = np.frombuffer(blob[8:20], dtype="<u2")
    _pmyzoizys, _npwofh, _txaetjsn, _hqqhuzdu, _zncbwpgl, _jyybalp = (int(_pmyzoizys), int(_npwofh), int(_txaetjsn), int(_hqqhuzdu), int(_zncbwpgl), int(_jyybalp))
    _pazxzil = bool(_bmboczbig & _huusbs)
    _iwmvinv = (blob[7] >> 1) & 7
    if blob[7] & 0xF0 or _iwmvinv > 4 or (_iwmvinv and not _pazxzil):
        raise ValueError("invalid fixed scale stage")
    if _pazxzil and (_wnlvmhec != 4 or not _txaetjsn or not _hqqhuzdu or _txaetjsn > _pmyzoizys or _hqqhuzdu > _npwofh or (_txaetjsn & 3) or (_hqqhuzdu & 3)):
        raise ValueError("invalid scaled EFC geometry")
    _kytqpjtx = (0, 5, 6, 8, 16)[_iwmvinv]
    if _kytqpjtx and (
        _txaetjsn != ((_pmyzoizys + _kytqpjtx - 1) // _kytqpjtx) * 4
        or _hqqhuzdu != ((_npwofh + _kytqpjtx - 1) // _kytqpjtx) * 4
    ):
        raise ValueError("fixed scale stage does not match geometry")
    _kyonerghc, _cgqzprh = (_txaetjsn, _hqqhuzdu) if _pazxzil else (_pmyzoizys, _npwofh)
    _ehvhaps = 2 if _bmboczbig & _bhbrejner else 1
    _jywtexeh = 20 + _jyybalp * 2 + _zncbwpgl * 9
    if _jywtexeh > len(blob):
        raise ValueError("truncated EFC header")
    if _ehvhaps == 1:
        _wcsjfna = np.frombuffer(blob[20:20 + _jyybalp * 2], dtype="<u2").copy()
    else:
        _wcsjfna = np.empty(0, np.uint16)
    _gazjxqcgp = []
    _wjboffi = 20 + _jyybalp * 2
    for _bdapholbj in range(_zncbwpgl):
        _kuhpqhrh = _wjboffi + _bdapholbj * 9
        _mtscsfe = int(np.frombuffer(blob[_kuhpqhrh + 1:_kuhpqhrh + 5], dtype="<u4")[0])
        _buecxk = int(np.frombuffer(blob[_kuhpqhrh + 5:_kuhpqhrh + 9], dtype="<u4")[0])
        _gazjxqcgp.append((_mtscsfe, _buecxk))
    _fduycpzwr = blob[_jywtexeh:]
    _segltpfe = _hoytyofzp = None
    if _bmboczbig & _qgbcnn:
        _wnuekdv, _segltpfe, _hoytyofzp = _sozuett(_fduycpzwr, bool(_bmboczbig & _emubcrdl))
    elif _bmboczbig & _emubcrdl:
        if len(_fduycpzwr) < 4:
            raise ValueError("truncated compressed stream")
        _wnuekdv = zlib.decompress(_fduycpzwr[4:], -15)
    else:
        _wnuekdv = _fduycpzwr
    _fhdihubdk = 0
    for _mtscsfe, _buecxk in _gazjxqcgp:
        _fhdihubdk = max(_fhdihubdk, _mtscsfe + _buecxk)
    if len(_wnuekdv) < _fhdihubdk:
        raise ValueError("truncated EFC stream")
    frames, color_key = _yqjucw(
        _wnuekdv, _gazjxqcgp, _wcsjfna, _ehvhaps, _kyonerghc, _cgqzprh, _wnlvmhec, _segltpfe, _hoytyofzp
    )
    if (blob[7] & 1) and _ehvhaps == 2:
        color_key = 0xF81F
    if _pazxzil:
        frames = _zydrbhgib(frames, _pmyzoizys, _npwofh, color_key, _kytqpjtx)
    _vlftjcgbz = {
        "width": _pmyzoizys,
        "height": _npwofh,
        "coded_width": _kyonerghc,
        "coded_height": _cgqzprh,
        "scaled": _pazxzil,
        "scale_output_tile": _kytqpjtx,
        "frame_count": _zncbwpgl,
        "tile": _wnlvmhec,
        "token_width": _ehvhaps,
        "palette_count": _jyybalp,
        "color_key": color_key,
        "compressed": bool(_bmboczbig & _emubcrdl),
    }
    return frames, _vlftjcgbz
